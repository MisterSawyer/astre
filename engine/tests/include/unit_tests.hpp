#pragma once

#include <optional>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <asio.hpp>

using namespace std::chrono_literals;

namespace astre::tests
{
    class BaseUnitTest : public testing::Test
    {
        public:
            virtual ~BaseUnitTest() = default;

            static void SetUpTestSuite()
            {
                spdlog::set_level(spdlog::level::debug);
            }

            static void TearDownTestSuite()
            {  
            }
    };

    class UnitTest : public BaseUnitTest
    {
        public:
            UnitTest() = default;
    };


    template <typename T>
    auto sync_await(asio::awaitable<T> && awaitable) {
        asio::io_context ctx;
        std::optional<T> result;
        asio::co_spawn(ctx, [&]() -> asio::awaitable<void> {
            result = co_await std::forward<typename asio::awaitable<T>>(awaitable);
        }, asio::detached);
        ctx.run();
        return *result;
    }

    template <typename T>
    auto sync_await(asio::io_context & ctx, asio::awaitable<T> && awaitable) {
        std::optional<T> result;
        asio::co_spawn(ctx, [&]() -> asio::awaitable<void> {
            result = co_await std::forward<typename asio::awaitable<T>>(awaitable);
        }, asio::detached);
        ctx.run();
        ctx.restart();
        return *result;
    }

    inline void sync_await(asio::awaitable<void> && awaitable) {
        asio::io_context ctx;
        asio::co_spawn(ctx, [&]() -> asio::awaitable<void> {
            co_await std::forward<typename asio::awaitable<void>>(awaitable);
        }, asio::detached);
        ctx.run();
    }

    inline void sync_await(asio::io_context & ctx, asio::awaitable<void> && awaitable) {
        asio::co_spawn(ctx, [&]() -> asio::awaitable<void> {
            co_await std::forward<typename asio::awaitable<void>>(awaitable);
        }, asio::detached);
        ctx.run();
        ctx.restart();
    }

}