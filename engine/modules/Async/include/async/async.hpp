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
#include <asio/experimental/parallel_group.hpp>

//#include <asio/experimental/awaitable_operators.hpp>
//using namespace asio::experimental::awaitable_operators;
#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>

/*
    co_stop_if this token requests it
*/
#define co_stop_if(token, ...) if ((token).stopRequested()) co_return __VA_ARGS__

namespace astre::async
{
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

        const base & executor() const noexcept { return *this; }

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

            AsyncContext<asio::io_context> & getAsyncContext();

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
        void requestStop() noexcept {stop.test_and_set();}

        /**
         * Check if the current task has been requested to stop
         */
        bool stopRequested() const noexcept {return stop.test();}

        /**
         * Mark the current task as finished
         */
        void markFinished() noexcept { finished.test_and_set(); }

        /**
         * Check if the current task has finished
         */
        bool isFinished() const noexcept { return finished.test(); }

        private:
            std::atomic_flag stop = ATOMIC_FLAG_INIT;
            std::atomic_flag finished = ATOMIC_FLAG_INIT;
    };
}