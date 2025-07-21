#pragma once

#include <functional>

#include <asio.hpp>

namespace astre::process
{
    struct WindowCallbacks
    {
        asio::any_io_executor executor;
        
        std::function<asio::awaitable<void>()> onDestroy;
        std::function<asio::awaitable<void>(int, int)> onResize;
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