#pragma once

#include <functional>

#include <asio.hpp>
#include "async/async.hpp"

namespace astre::process
{
    struct WindowCallbacks
    {
        async::AsyncContext<asio::thread_pool> context;
        
        std::function<asio::awaitable<void>()> onDestroy;
        std::function<asio::awaitable<void>(unsigned int, unsigned int)> onResize;
        std::function<asio::awaitable<void>()> onMove;
        std::function<asio::awaitable<void>()> onFocus;
        std::function<asio::awaitable<void>()> onUnfocus;
        std::function<asio::awaitable<void>(int)> onKeyPress;
        std::function<asio::awaitable<void>(int)> onKeyRelease;
        std::function<asio::awaitable<void>(int)> onMouseButtonDown;
        std::function<asio::awaitable<void>(int)> onMouseButtonUp;
        std::function<asio::awaitable<void>(int, int, float, float)> onMouseMove;
    };
}