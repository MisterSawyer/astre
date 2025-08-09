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

TEST(LifecycleTokenTest, SupportsMultipleFinishes) {
    LifecycleToken token;
    token.markFinished();
    token.markFinished(); // Should be idempotent
    EXPECT_TRUE(token.isFinished());
}

TEST(LifecycleTokenTest, CoroutineCompletesWithoutCancellation)
{
    asio::io_context io;
    std::atomic<bool> completed = false;

    asio::co_spawn(io, completes_normally(completed), asio::detached);
    io.run();

    EXPECT_TRUE(completed);
}
