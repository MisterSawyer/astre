#include <gtest/gtest.h>
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
};

template<class TestImplType>
struct TestModel final : public ModelBase<TestInterface, TestImplType>
{
    using base = ModelBase<TestInterface, TestImplType>;
    using const_base = const base;

    template<class... Args>                                                                             
    inline TestModel(Args && ... args) : base(std::forward<Args>(args)...){} 

    int value() const override { return base::impl().value(); }

    void move(InterfaceBase * dest) override
    {
        ::new(dest) TestModel(std::move(base::impl()));
    }

    void copy(InterfaceBase * dest) const override
    {
        ::new(dest) TestModel(const_base::impl());
    }

    std::unique_ptr<InterfaceBase> clone() const override
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
    Implementation<TestInterface, TestModel> impl{LargeType{17}};
    ASSERT_TRUE(bool(impl));
    EXPECT_EQ(impl->value(), 17);
}

TEST(SBOTest, ImplementationMoveSemantics) {
    Implementation<TestInterface, TestModel> a{SmallType{123}};
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
class TrackedModel : public ModelBase<TestInterface, Impl> {
public:
    using base = ModelBase<TestInterface, Impl>;
    using const_base = const base;

    template<typename... Args>
    TrackedModel(Args&&... args) : base(std::forward<Args>(args)...) {}

    int value() const override {
        return base::impl().value();
    }

    ~TrackedModel() override = default;

    void move(InterfaceBase * dest) override
    {
        ::new(dest) TrackedModel(std::move(base::impl()));
    }

    void copy([[maybe_unused]] InterfaceBase * dest) const override
    {
        throw std::runtime_error("Not copyable");
    }

    std::unique_ptr<InterfaceBase> clone() const override
    {
        throw std::runtime_error("Not copyable");
    }
};

TEST_F(SBOTypeSafetyTest, MoveDoesNotCauseDoubleFree) {
    {
        SBO<TestInterface, TrackedModel> a{std::in_place_type<TrackedType>, 7};
        EXPECT_EQ(TrackedType::constructions, 1);
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<TestInterface, TrackedModel> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 7);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    EXPECT_EQ(TrackedType::moves, 1);
}

TEST_F(SBOTypeSafetyTest, HeapAllocatedMoveIsSafe) {
    {
        SBO<TestInterface, TrackedModel, 1> a{std::in_place_type<TrackedType>, 13}; // forced heap alloc
        EXPECT_TRUE(a.isHeapAllocated());

        SBO<TestInterface, TrackedModel, 1> b = std::move(a);
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
        SBO<TestInterface, TrackedModel, 16> a{std::in_place_type<TrackedType>, 13}; // forced buffer alloc
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<TestInterface, TrackedModel, 16> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 13);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    EXPECT_EQ(TrackedType::moves, 1);
}

TEST_F(SBOTypeSafetyTest, SBODestructorDoesNotDoubleFreeMoved) {
    SBO<TestInterface, TrackedModel> a{std::in_place_type<TrackedType>, 21};
    EXPECT_EQ(TrackedType::constructions, 1);

    SBO<TestInterface, TrackedModel> b = std::move(a);
    (void)b;

    // after scope, `a` should not double free
}

TEST_F(SBOTypeSafetyTest, DeathOnCopyingNonCopyableHeap) {
    const SBO<TestInterface, TrackedModel, 1> a{std::in_place_type<TrackedType>, 21};
    EXPECT_TRUE(a.isHeapAllocated());
    auto cause_death = [&a]() {
        [[maybe_unused]] SBO<TestInterface, TrackedModel, 1> b(a);
    };

    EXPECT_DEATH(cause_death(), ".*");
}

TEST_F(SBOTypeSafetyTest, DeathOnCopyingNonCopyableBuffer) {
    const SBO<TestInterface, TrackedModel> a{std::in_place_type<TrackedType>, 21};
    EXPECT_FALSE(a.isHeapAllocated());
    auto cause_death = [&a]() {
        [[maybe_unused]] SBO<TestInterface, TrackedModel> b(a);
    };

    EXPECT_DEATH(cause_death(), ".*");
}