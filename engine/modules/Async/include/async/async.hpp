#pragma once

#include <utility>
#include <exception>
#include <stdexcept>
#include <variant>
#include <unordered_map>

#include <chrono>
using namespace std::chrono_literals;
#include <thread>

#include "native/native.h"
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
#include <asio/experimental/concurrent_channel.hpp>
#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>

namespace astre::async
{
    template <std::size_t... Is, typename T>
    asio::awaitable<void> await_all_impl(std::index_sequence<Is...>, std::array<T, sizeof...(Is)> arr)
    {
        // Co_await all concurrently using awaitable && operator
        return (std::move(arr.at(Is)) && ...);
    }

    template <typename T, std::size_t N>
    asio::awaitable<void> await_all(std::array<T, N> arr)
    {
        if constexpr (N == 0)
            return;
        else
            return await_all_impl(std::make_index_sequence<N>{}, std::move(arr));
    }


    inline asio::awaitable<void> noop()
    {
        co_return;
    }

    // tells where async operations should run
    template<class AsioContext>
    class AsyncContext : public asio::strand<typename AsioContext::executor_type> {
    public:
        using executor_type = typename AsioContext::executor_type;
        using base = asio::strand<executor_type>;

        explicit AsyncContext(AsioContext& ctx)
            : base(asio::make_strand(ctx.get_executor())) {}

        // async context is not copyable
        AsyncContext(const AsyncContext&) = delete;
        AsyncContext& operator=(const AsyncContext&) = delete;

        AsyncContext(AsyncContext&&) noexcept = default;
        AsyncContext& operator=(AsyncContext&&) noexcept = default;

        asio::awaitable<void> ensureOnStrand() const {
            if (!this->running_in_this_thread()) {
                return asio::dispatch(asio::bind_executor(static_cast<base>(*this), asio::use_awaitable));
            }
            return noop();
        }
    };

    template<class Executor>
    AsyncContext(Executor & ) -> AsyncContext<Executor>;

    class ThreadContext : public asio::io_context
    {
        public:
            ThreadContext();

            virtual ~ThreadContext();

            asio::awaitable<void> ensureOnStrand() const;

            template<class Func, class... Args>
            void start(Func && func, Args&&... args)
            {
                _thread = std::thread(std::forward<Func>(func), std::forward<Args>(args)...);
            }

            void join();

            void close();
        
            bool running() const;

        private:
            asio::executor_work_guard<asio::io_context::executor_type> _work_guard;
            AsyncContext<asio::io_context> _async_context;

            std::thread _thread;
    };


    class LifecycleToken 
    {
        public:

        LifecycleToken()
        {
            spdlog::debug("[lifecycle-token] LifecycleToken created");
        }
        
        // token is non transferable  
        LifecycleToken(LifecycleToken && other) = delete;
        LifecycleToken(LifecycleToken & other) = delete;
        LifecycleToken & operator=(LifecycleToken && other) = delete;
        LifecycleToken & operator=(LifecycleToken & other) = delete;

        ~LifecycleToken() 
        {
            spdlog::debug("[lifecycle-token] LifecycleToken destroyed");
        }

        /**
         * Request the cancellation of the current task
         */
        void requestStop() noexcept {
            signal.emit(asio::cancellation_type::all);
        }

        /**
         * Mark the current task as finished
         */
        void markFinished() noexcept { finished.test_and_set(); }

        /**
         * Check if the current task is finished
         */
        bool isFinished() const noexcept { return finished.test(); }

        asio::cancellation_slot getSlot() { return signal.slot(); }
        asio::cancellation_signal & getSignal() { return signal; }

        private:
            asio::cancellation_signal signal;
            std::atomic_flag finished = ATOMIC_FLAG_INIT;
    };
}