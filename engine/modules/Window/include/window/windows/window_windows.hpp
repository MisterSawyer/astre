#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "async/async.hpp"
#include "process/process.hpp"

namespace astre::window::windows
{
    /**
     * @brief WinAPI `IWindow` interface implementation
     */
    class WinapiWindow
    {
        public:
            using execution_context_type = process::IProcess::execution_context_type;

            WinapiWindow(process::IProcess & process, native::window_handle window_handle, unsigned int width, unsigned int height);

            WinapiWindow(WinapiWindow && other);
            
            WinapiWindow(const WinapiWindow & other) = delete;

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
        
            async::AsyncContext<execution_context_type> _async_context;

            process::IProcess & _process;

            native::window_handle _window_handle;
            unsigned int _width, _height;

            std::optional<native::device_context> _acquired_device_context;
    };
}