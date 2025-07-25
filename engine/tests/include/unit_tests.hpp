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

    template <typename ExecutionContextType, typename T>
    T sync_await(ExecutionContextType & ctx, asio::awaitable<T> && awaitable)
    {
        std::promise<T> promise;

        asio::co_spawn(ctx,
            [&]() -> asio::awaitable<void> {
                try {
                    T value = co_await std::move(awaitable);
                    promise.set_value(std::move(value));
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
                co_return;
            },
            asio::detached
        );

        return promise.get_future().get();
    }

    template <typename ExecutionContextType>
    void sync_await(ExecutionContextType & ctx, asio::awaitable<void> && awaitable)
    {
        std::promise<void> promise;

        asio::co_spawn(ctx, 
            [&]() -> asio::awaitable<void> {
                try {
                    co_await std::move(awaitable);
                    promise.set_value();
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            },
            asio::detached
        );

        promise.get_future().get();
    }

}