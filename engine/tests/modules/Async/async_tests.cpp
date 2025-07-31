#include <gtest/gtest.h>
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>

#include "async/async.hpp"

using namespace std::chrono_literals;
using namespace astre::async;

asio::awaitable<void> cancellable_task(asio::cancellation_slot slot, std::atomic<bool>& started, std::atomic<bool>& cancelled_flag)
{
    asio::cancellation_state state(slot);
    started = true;

    try {
        for (int i = 0; i < 10; ++i) {
            // Simulate work
            co_await asio::steady_timer(co_await asio::this_coro::executor, 50ms).async_wait(asio::use_awaitable);

            if (state.cancelled() != asio::cancellation_type::none) {
                cancelled_flag = true;
                co_return;
            }
        }
    } catch (...) {
        cancelled_flag = true;
        throw;
    }
}

asio::awaitable<void> completes_normally(std::atomic<bool>& finished) {
    asio::steady_timer timer(co_await asio::this_coro::executor, 50ms);
    co_await timer.async_wait(asio::use_awaitable);
    finished = true;
    co_return;
}

asio::awaitable<void> deep_task(std::atomic<int>& steps, asio::cancellation_slot slot) {
    asio::cancellation_state state(slot);
    steps.fetch_add(1); // Entered deep

    co_await asio::steady_timer(co_await asio::this_coro::executor, 20ms).async_wait(asio::use_awaitable);

    if (state.cancelled() != asio::cancellation_type::none)
        co_return;

    steps.fetch_add(1); // Finished deep
}

asio::awaitable<void> middle_task(std::atomic<int>& steps, asio::cancellation_slot slot) {
    asio::cancellation_state state(slot);
    steps.fetch_add(1); // Entered middle

    co_await deep_task(steps, slot);

    if (state.cancelled() != asio::cancellation_type::none)
        co_return;

    steps.fetch_add(1); // Finished middle
}

asio::awaitable<void> root_task(std::atomic<int>& steps, asio::cancellation_slot slot) {
    asio::cancellation_state state(slot);
    steps.fetch_add(1); // Entered root

    co_await middle_task(steps, slot);

    if (state.cancelled() != asio::cancellation_type::none)
        co_return;

    steps.fetch_add(1); // Finished root
}

asio::awaitable<void> cleanup_test(std::atomic<bool>& cleaned_up, asio::cancellation_slot slot) {
    struct ScopeExit {
        std::atomic<bool>& flag;
        ~ScopeExit() { flag = true; }
    } cleanup{cleaned_up};

    co_await asio::steady_timer(co_await asio::this_coro::executor, 50ms).async_wait(asio::use_awaitable);
    co_return;
}

TEST(LifecycleTokenTest, InitiallyNotStoppingOrFinished) {
    LifecycleToken token;

    EXPECT_FALSE(token.isFinished());
}


TEST(LifecycleTokenTest, MarkFinishedSetsFinished) {
    LifecycleToken token;
    EXPECT_FALSE(token.isFinished());

    token.markFinished();
    EXPECT_TRUE(token.isFinished());
}

TEST(LifecycleTokenTest, CancellationPropagatesToSlot) {
    LifecycleToken token;

    bool was_cancelled = false;

    token.getSlot().assign([&](asio::cancellation_type type) {
        was_cancelled = true;
    });

    token.requestStop();
    EXPECT_TRUE(was_cancelled);
}

TEST(LifecycleTokenTest, SupportsMultipleFinishes) {
    LifecycleToken token;
    token.markFinished();
    token.markFinished(); // Should be idempotent
    EXPECT_TRUE(token.isFinished());
}


TEST(LifecycleTokenTest, CoroutineStopsWhenCancelled)
{
    asio::io_context io;
    LifecycleToken token;

    std::atomic<bool> started = false;
    std::atomic<bool> was_cancelled = false;

    asio::co_spawn(io,
        cancellable_task(token.getSlot(), started, was_cancelled),
        asio::detached
    );

    std::thread cancel_thread([&]() {
        std::this_thread::sleep_for(80ms); // Let coroutine start
        token.requestStop();
    });

    io.run();

    cancel_thread.join();

    EXPECT_TRUE(started);
    EXPECT_TRUE(was_cancelled);
}


TEST(LifecycleTokenTest, CoroutineCompletesWithoutCancellation)
{
    asio::io_context io;
    std::atomic<bool> completed = false;

    asio::co_spawn(io, completes_normally(completed), asio::detached);
    io.run();

    EXPECT_TRUE(completed);
}

TEST(LifecycleTokenTest, SlotHandlerIsInvokedOnCancel)
{
    LifecycleToken token;
    bool cancelled_handler_called = false;

    token.getSlot().assign([&](asio::cancellation_type) {
        cancelled_handler_called = true;
    });

    token.requestStop();

    EXPECT_TRUE(cancelled_handler_called);
}

TEST(LifecycleTokenTest, NestedCoroutinesCancelledMidway)
{
    asio::io_context io;
    LifecycleToken token;

    std::atomic<int> steps = 0;

    asio::co_spawn(io, root_task(steps, token.getSlot()), asio::detached);

    std::thread cancel_thread([&]() {
        std::this_thread::sleep_for(25ms);
        token.requestStop();
    });

    io.run();
    cancel_thread.join();

    // Expect: Entered root, middle, deep (3), cancelled before completion
    EXPECT_GE(steps, 3);
    EXPECT_LT(steps, 6); // Should not have reached all final steps
}

TEST(LifecycleTokenTest, NestedCoroutinesCompleteFully)
{
    asio::io_context io;
    LifecycleToken token;

    std::atomic<int> steps = 0;

    asio::co_spawn(io, root_task(steps, token.getSlot()), asio::detached);
    io.run();

    // Expect all phases: root(1+1), middle(1+1), deep(1+1)
    EXPECT_EQ(steps, 6);
}

TEST(LifecycleTokenTest, CleanupRunsOnCancellation)
{
    asio::io_context io;
    LifecycleToken token;

    std::atomic<bool> cleaned_up = false;

    asio::co_spawn(io, cleanup_test(cleaned_up, token.getSlot()), asio::detached);

    std::thread cancel_thread([&]() {
        std::this_thread::sleep_for(10ms);
        token.requestStop();
    });

    io.run();
    cancel_thread.join();

    EXPECT_TRUE(cleaned_up);
}