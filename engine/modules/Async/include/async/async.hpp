#pragma once

#include "native/native.h"
#include <asio.hpp>

#include <chrono>
using namespace std::chrono_literals;

namespace astre::async
{
    inline asio::awaitable<void> noop()
    {
        co_return;
    }

    template<class AsioContext>
    class AsyncContext
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

            inline asio::awaitable<void> ensureOnStrand()
            {
                if(!_strand.running_in_this_thread()) {
                    return asio::dispatch(asio::bind_executor(_strand, asio::use_awaitable));
                }
                return noop();
            }

            template<typename Awaitable>
            void co_spawn(Awaitable&& awaitable)
            {
                asio::co_spawn(_strand, std::forward<Awaitable>(awaitable), asio::detached);
            }

        private:
            asio::strand<executor_type> _strand;
    };


}