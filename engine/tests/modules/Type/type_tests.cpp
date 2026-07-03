#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "type/type.hpp"

using namespace astre::type;

// Define a minimal interface with virtual behavior
class TestInterface : public InterfaceBase {
public:
    ~TestInterface()
    {
        spdlog::debug("TestInterface dtor");
    }
    virtual int value() const = 0;

    virtual void move(TestInterface * dest)  = 0;

    virtual void copy([[maybe_unused]] TestInterface * dest) const = 0;

    virtual std::unique_ptr<TestInterface> clone() const = 0;
};

class MoveOnlyTestInterface : public InterfaceBase {
public:
    ~MoveOnlyTestInterface()
    {
        spdlog::debug("MoveOnlyTestInterface dtor");
    }
    virtual int value() const = 0;

    virtual void move(MoveOnlyTestInterface * dest) = 0;
};

template<class TestImplType>
struct TestModel final : public ModelBase<TestInterface, TestImplType>
{
    using base = ModelBase<TestInterface, TestImplType>;
    using const_base = const base;

    template<class... Args>                                                                             
    inline TestModel(Args && ... args) : base(std::forward<Args>(args)...){} 

    int value() const override { return base::impl().value(); }

    void move(TestInterface * dest) override
    {
        ::new(dest) TestModel(std::move(base::impl()));
    }

    void copy(TestInterface * dest) const override
    {
        ::new(dest) TestModel(const_base::impl());
    }

    std::unique_ptr<TestInterface> clone() const override
    {
        return std::make_unique<TestModel>(base::impl());
    }
};

// Implementation that fits in the SBO buffer
class SmallType {
public:
    SmallType(int v) : _v(v) {}
    int value() const { return _v; }

    ~SmallType()
    {
    }
private:
    int _v;
};

// Large implementation that *exceeds* the SBO buffer
class LargeType {
public:
    LargeType(int v) : _v(v) {}
    int value() const { return _v; }

    ~LargeType()
    {
    }

private:
    int _v;
    std::array<char, 256> padding{}; // force heap fallback
};

struct TrackedType {
    static inline int constructions = 0;
    static inline int destructions = 0;
    static inline int moves = 0;
    static inline bool was_destroyed = false;

    int _val = 0;
    bool _moved = false;

    TrackedType(int v) : _val(v) {
        constructions++;
    }

    TrackedType(TrackedType&& other) noexcept {
        _val = other._val;
        _moved = false;
        other._moved = true;
        moves++;
    }

    TrackedType& operator=(TrackedType&& other) noexcept {
        if (this != &other) {
            _val = other._val;
            _moved = false;
            other._moved = true;
            moves++;
        }
        return *this;
    }

    TrackedType(const TrackedType&) = delete;
    TrackedType& operator=(const TrackedType&) = delete;

    int value() const {
        if (_moved)
            throw std::runtime_error("Accessing moved-from TrackedType");
        return _val;
    }

    ~TrackedType() {
        if (_moved) return;  // moved-from shouldn't count
        destructions++;
        was_destroyed = true;
    }

    static void reset() {
        constructions = 0;
        destructions = 0;
        moves = 0;
        was_destroyed = false;
    }
};


TEST(SBOTest, SmallObjectStoredInPlace) {
    SBO<TestInterface, TestModel> sbo = SBO<TestInterface, TestModel, 128>(std::in_place_type<SmallType>, 42);
    ASSERT_TRUE(bool(sbo));
    EXPECT_EQ(sbo->value(), 42);
    EXPECT_FALSE(sbo.isHeapAllocated());
}

TEST(SBOTest, LargeObjectStoredOnHeap) {
    SBO<TestInterface, TestModel, 64> sbo = SBO<TestInterface, TestModel, 64>(std::in_place_type<LargeType>, 99);
    ASSERT_TRUE(bool(sbo));
    EXPECT_EQ(sbo->value(), 99);
    EXPECT_TRUE(sbo.isHeapAllocated());
}

TEST(SBOTest, ImplementationWrapperWorks) {
    Implementation<TestInterface, TestModel> impl{std::in_place_type<LargeType>, 17};
    ASSERT_TRUE(bool(impl));
    EXPECT_EQ(impl->value(), 17);
}

TEST(SBOTest, ImplementationMoveSemantics) {
    Implementation<TestInterface, TestModel> a{std::in_place_type<SmallType>, 123};
    Implementation<TestInterface, TestModel> b = std::move(a);

    EXPECT_TRUE(bool(b));
    EXPECT_EQ(b->value(), 123);
    EXPECT_FALSE(static_cast<bool>(a)); // likely null after move
}


TEST(SBOTest, SBODestructorIsCalled) {
    {
        SBO<TestInterface, TestModel> sbo{std::in_place_type<SmallType>, 5};
        EXPECT_EQ(sbo->value(), 5);
    }
    SUCCEED();  // No crash = destructor ran successfully
}

TEST(SBOTest, CopiesHeapAllocatedObject) {
    const SBO<TestInterface, TestModel, 64> a{std::in_place_type<LargeType>, 21};
    EXPECT_TRUE(a.isHeapAllocated());

    SBO<TestInterface, TestModel, 64> b(a);
    ASSERT_TRUE(b);
    EXPECT_TRUE(b.isHeapAllocated());
    EXPECT_EQ(b->value(), 21);
    EXPECT_EQ(a->value(), 21);
}

TEST(SBOTest, CopiesSBOAllocatedObject) {
    const SBO<TestInterface, TestModel> a{std::in_place_type<SmallType>, 21};
    EXPECT_FALSE(a.isHeapAllocated());

    SBO<TestInterface, TestModel> b(a);
    ASSERT_TRUE(b);
    EXPECT_FALSE(b.isHeapAllocated());
    EXPECT_EQ(b->value(), 21);
    EXPECT_EQ(a->value(), 21);
}

class SBOTypeSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        TrackedType::reset();
    }

    void TearDown() override {
        EXPECT_EQ(TrackedType::constructions, TrackedType::destructions)
            << "Construction and destruction mismatch (possible leak or double free)";
    }
};

template<class Impl>
class MoveOnlyTrackedModel : public ModelBase<MoveOnlyTestInterface, Impl> {
public:
    using base = ModelBase<MoveOnlyTestInterface, Impl>;

    template<typename... Args>
    MoveOnlyTrackedModel(Args&&... args) : base(std::forward<Args>(args)...) {}

    int value() const override {
        return base::impl().value();
    }

    ~MoveOnlyTrackedModel() override = default;

    void move(MoveOnlyTestInterface * dest) override
    {
        ::new(dest) MoveOnlyTrackedModel(std::move(base::impl()));
    }
};

TEST_F(SBOTypeSafetyTest, MoveDoesNotCauseDoubleFree) {
    {
        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel> a{std::in_place_type<TrackedType>, 7};
        EXPECT_EQ(TrackedType::constructions, 1);
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 7);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    EXPECT_EQ(TrackedType::moves, 1);
}

TEST_F(SBOTypeSafetyTest, HeapAllocatedMoveIsSafe) {
    {
        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel, 1> a{std::in_place_type<TrackedType>, 13}; // forced heap alloc
        EXPECT_TRUE(a.isHeapAllocated());

        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel, 1> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 13);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    // we are moving a heap allocated object
    // so it should not be counted as a move
    // because all we do is move the pointer
    EXPECT_EQ(TrackedType::moves, 0);
}

TEST_F(SBOTypeSafetyTest, SBOAllocatedMoveIsSafe) {
    {
        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel, 16> a{std::in_place_type<TrackedType>, 13}; // forced buffer alloc
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel, 16> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 13);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    EXPECT_EQ(TrackedType::moves, 1);
}

TEST_F(SBOTypeSafetyTest, SBODestructorDoesNotDoubleFreeMoved) {
    SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel> a{std::in_place_type<TrackedType>, 21};
    EXPECT_EQ(TrackedType::constructions, 1);

    SBO<MoveOnlyTestInterface, MoveOnlyTrackedModel> b = std::move(a);
    (void)b;

    // after scope, `a` should not double free
}
