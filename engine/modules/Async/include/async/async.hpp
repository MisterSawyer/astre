#pragma once

#include "native/native.h"
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;

#include <chrono>
using namespace std::chrono_literals;
#include <thread>

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
            inline AsyncContext(const AsyncContext &) = default;
            inline AsyncContext(AsyncContext &&) = default;
            inline AsyncContext & operator=(const AsyncContext &) = default;
            inline AsyncContext & operator=(AsyncContext &&) = default;

            inline asio::awaitable<void> ensureOnStrand() const
            {
                if(!_strand.running_in_this_thread()) {
                    return asio::dispatch(asio::bind_executor(_strand, asio::use_awaitable));
                }
                return noop();
            }

            template<typename T>
            void co_spawn(asio::awaitable<T> && awaitable) const
            {
                asio::co_spawn(_strand, std::forward<typename asio::awaitable<T>>(awaitable), asio::detached);
            }

        private:
            asio::strand<executor_type> _strand;
    };

    class ThreadContext
    {
        public:
            ThreadContext();

            virtual ~ThreadContext();

            asio::awaitable<void> ensureOnStrand() const;

            template<typename Awaitable>
            void co_spawn(Awaitable&& awaitable) const
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
            
            void run();
            
            void poll();

        private:
            asio::io_context _io_context;
            asio::executor_work_guard<asio::io_context::executor_type> _work_guard;
            AsyncContext<asio::io_context> _async_context;

            std::thread _thread;
    };

}