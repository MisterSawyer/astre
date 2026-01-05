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

/**
 * @brief Early-return from a coroutine when a stop is requested.
 *
 * Evaluates the token’s stop flag and, if set, immediately `co_return`s.
 * Optional return values can be passed through the variadic args.
 *
 * Example:
 *   co_stop_if(token);           // co_return;
 *   co_stop_if(token, false);    // co_return false;
 *
 * Notes:
 * - Intended for use inside `asio::awaitable`/coroutine bodies.
 * - If no return value is provided, the coroutine’s return type must be `void`.
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

        executor_type executor() const noexcept { return base::get_inner_executor(); }

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
            /// Construct io_context with a work guard and strand; thread not started
            ThreadContext();
            
            /// Join the worker thread if joinable; call close() first to let run() exit.
            virtual ~ThreadContext();

            /// Awaitable that dispatches to the strand if called off-strand.
            asio::awaitable<void> ensureOnStrand() const;

            /// Spawn the worker thread running the provided callable
            template<class Func, class... Args>
            void start(Func && func, Args&&... args)
            {
                _thread = std::thread(std::forward<Func>(func), std::forward<Args>(args)...);
            }

            /// Block until the worker thread exits if joinable.
            void join();

            /// Release the work guard so io_context::run can exit once work drains.
            void close();

            /// True when a thread is joinable and the work guard still owns work.
            bool running() const;
            
            /// Access the strand-bound AsyncContext for serialized async operations.
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