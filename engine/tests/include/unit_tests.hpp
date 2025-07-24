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
    auto sync_await(ExecutionContextType & ctx, asio::awaitable<T> && awaitable) 
    { 
        std::promise<bool> promise;
        std::optional<T> result;
        asio::co_spawn(ctx, 
            [&]() -> asio::awaitable<void> {
                spdlog::info("awaiting...");
                result = co_await std::move(awaitable);
                spdlog::info("sync_await finished");
            },
            [&promise](std::exception_ptr) {
                promise.set_value(true);
            }
        );
        promise.get_future().get();
        return std::move(*result);
    }

    template <typename ExecutionContextType>
    inline void sync_await(ExecutionContextType & ctx, asio::awaitable<void> && awaitable) {
        std::promise<bool> promise;
        asio::co_spawn(ctx, 
            [&]() -> asio::awaitable<void> {
                co_await std::move(awaitable);
            },
            [&promise](std::exception_ptr) {
                promise.set_value(true);
            }
        );
        promise.get_future().get();
    }

}