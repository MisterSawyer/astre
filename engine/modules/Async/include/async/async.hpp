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
    inline asio::awaitable<void> noop()
    {
        co_return;
    }

    template<class AsioContext>
    class AsyncContext final
    {
        public:
            using context_type = AsioContext;
            using executor_type = typename context_type::executor_type;
        
            explicit inline AsyncContext(context_type & context) 
                : _strand(asio::make_strand(context.get_executor()))
            {}

            inline AsyncContext(AsyncContext && other)
            : _strand(std::move(other._strand))
            {}

            inline AsyncContext & operator=(AsyncContext && other)
            {
                if(this == &other) return *this;
                _strand = std::move(other._strand);
                return *this;
            }

            AsyncContext(const AsyncContext & other) = delete;

            AsyncContext & operator=(const AsyncContext & other) = delete;

            // awaitable factory
            inline asio::awaitable<void> ensureOnStrand() const
            {
                if(!_strand.running_in_this_thread()) {
                    return asio::dispatch(asio::bind_executor(_strand, asio::use_awaitable));
                }
                return noop();
            }

            // spawn new coroutine
            template<typename T>
            inline void co_spawn(asio::awaitable<T> && awaitable)
            {
                assert(awaitable.valid() && "awaitable is invalid (possibly moved-from)");

                asio::co_spawn(_strand, std::forward<typename asio::awaitable<T>>(awaitable), asio::detached);
            }

        private:
            asio::strand<executor_type> _strand;
    };

    template<class Executor>
    AsyncContext(Executor & ) -> AsyncContext<Executor>;

    class ThreadContext : public asio::io_context
    {
        public:
            ThreadContext();

            virtual ~ThreadContext();

            asio::awaitable<void> ensureOnStrand() const;

            template<typename Awaitable>
            void co_spawn(Awaitable&& awaitable)
            {
                _async_context.co_spawn(std::move(awaitable));
            }

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

}