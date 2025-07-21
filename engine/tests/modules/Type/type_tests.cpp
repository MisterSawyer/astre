#include <gtest/gtest.h>
#include "type/type.hpp"

using namespace astre::type;

// Define a minimal interface with virtual behavior
class TestInterface : public Interface {
public:
    ~TestInterface()
    {
        spdlog::debug("TestInterface dtor");
    }
    virtual int value() const = 0;
};

template<class TestImplType>
struct TestDispatcher final : public Dispatcher<TestInterface, TestImplType>
{
    using dispatch = Dispatcher<TestInterface, TestImplType>;

    ~TestDispatcher()
    {
    }

    template<class... Args>                                                                             
    inline TestDispatcher(Args && ... args) : dispatch(std::forward<Args>(args)...){} 

    int value() const override { return dispatch::impl().value(); }
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
    SBO<TestInterface, TestDispatcher> sbo = SBO<TestInterface, TestDispatcher, 128>(std::in_place_type<SmallType>, 42);
    ASSERT_TRUE(bool(sbo));
    EXPECT_EQ(sbo->value(), 42);
    EXPECT_FALSE(sbo.isHeapAllocated());
}

TEST(SBOTest, LargeObjectStoredOnHeap) {
    SBO<TestInterface, TestDispatcher, 64> sbo = SBO<TestInterface, TestDispatcher, 64>(std::in_place_type<LargeType>, 99);
    ASSERT_TRUE(bool(sbo));
    EXPECT_EQ(sbo->value(), 99);
    EXPECT_TRUE(sbo.isHeapAllocated());
}

TEST(SBOTest, ImplementationWrapperWorks) {
    Implementation<TestInterface, TestDispatcher> impl{LargeType{17}};
    ASSERT_TRUE(bool(impl));
    EXPECT_EQ(impl->value(), 17);
}

TEST(SBOTest, ImplementationMoveSemantics) {
    Implementation<TestInterface, TestDispatcher> a{SmallType{123}};
    Implementation<TestInterface, TestDispatcher> b = std::move(a);

    EXPECT_TRUE(bool(b));
    EXPECT_EQ(b->value(), 123);
    EXPECT_FALSE(static_cast<bool>(a)); // likely null after move
}


TEST(SBOTest, SBODestructorIsCalled) {
    {
        SBO<TestInterface, TestDispatcher> sbo{std::in_place_type<SmallType>, 5};
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
class TrackedDispatcher : public Dispatcher<TestInterface, Impl> {
public:
    using Base = Dispatcher<TestInterface, Impl>;

    template<typename... Args>
    TrackedDispatcher(Args&&... args) : Base(std::forward<Args>(args)...) {}

    int value() const override {
        return this->impl().value();
    }

    ~TrackedDispatcher() override = default;
};

TEST_F(SBOTypeSafetyTest, MoveDoesNotCauseDoubleFree) {
    {
        SBO<TestInterface, TrackedDispatcher> a{std::in_place_type<TrackedType>, 7};
        EXPECT_EQ(TrackedType::constructions, 1);
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<TestInterface, TrackedDispatcher> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 7);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    EXPECT_EQ(TrackedType::moves, 0);
}

TEST_F(SBOTypeSafetyTest, HeapAllocatedMoveIsSafe) {
    {
        SBO<TestInterface, TrackedDispatcher, 1> a{std::in_place_type<TrackedType>, 13}; // forced heap alloc
        EXPECT_TRUE(a.isHeapAllocated());

        SBO<TestInterface, TrackedDispatcher, 1> b = std::move(a);
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
        SBO<TestInterface, TrackedDispatcher, 16> a{std::in_place_type<TrackedType>, 13}; // forced buffer alloc
        EXPECT_FALSE(a.isHeapAllocated());

        SBO<TestInterface, TrackedDispatcher, 16> b = std::move(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value(), 13);
    }

    EXPECT_EQ(TrackedType::constructions, TrackedType::destructions);
    // we are moving a buffer allocated object
    // so it should not be counted as a move
    // because all we do is move the buffer pointer
    EXPECT_EQ(TrackedType::moves, 0);
}

TEST_F(SBOTypeSafetyTest, SBODestructorDoesNotDoubleFreeMoved) {
    SBO<TestInterface, TrackedDispatcher> a{std::in_place_type<TrackedType>, 21};
    EXPECT_EQ(TrackedType::constructions, 1);

    SBO<TestInterface, TrackedDispatcher> b = std::move(a);
    (void)b;

    // after scope, `a` should not double free
}

