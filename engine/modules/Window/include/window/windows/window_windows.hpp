#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "process/process.hpp"
#include "formatter/formatter.hpp"

namespace astre::window::windows
{
    /**
     * @brief WinAPI `IWindow` interface implementation
     */
    class WinapiWindow
    {
        public:
            WinapiWindow(WinapiWindow && other);

            WinapiWindow(asio::io_context & io_context, process::IProcess & process, native::window_handle window_handle, unsigned int width, unsigned int height);

            ~WinapiWindow();

            bool good() const;

            asio::awaitable<void> show();

            asio::awaitable<void> hide();

            asio::awaitable<void> close();
            
            unsigned int getWidth() const;

            unsigned int getHeight() const;

            native::window_handle getHandle() const;

            native::device_context acquireDeviceContext();

            bool releaseDeviceContext(native::device_context device_context);

            process::IProcess & getProcess();

        private:
        
            asio::io_context & _io_context;
            asio::strand<asio::io_context::executor_type> _strand;

            process::IProcess & _process;

            native::window_handle _window_handle;
            unsigned int _width, _height;

            std::optional<native::device_context> _acquired_device_context;
    };
}