#include <spdlog/spdlog.h>

#include "formatter/formatter.hpp"
#include "window/window.hpp"

#include "window/windows/window_windows.hpp"

namespace astre::window
{
    /**
     * @brief Creates a new window
     * 
     * @return A new window
     */
    asio::awaitable<Window> createWindow(process::IProcess & process, const std::string & title, unsigned int width, unsigned int height)
    {
        auto window_handle = co_await process.registerWindow(title, width, height);
        co_return Window(std::in_place_type<windows::WinapiWindow>, process, window_handle, width, height);
    }
}
namespace astre::window::windows
{
    WinapiWindow::WinapiWindow(process::IProcess & process, native::window_handle window_handle, unsigned int width, unsigned int height)
    :   _async_context(process.getExecutionContext()),
        _process(process),
        _window_handle(window_handle),
        _width(width),
        _height(height)
        
    {
        spdlog::info("[window] Winapi window created");
    }

    WinapiWindow::WinapiWindow(WinapiWindow && other)
    :   _async_context(std::move(other._async_context)),
        _window_handle(other._window_handle),
        _process(other._process),
        _width(other._width),
        _height(other._height)
    {
        other._window_handle = nullptr;
    }

    WinapiWindow::~WinapiWindow()
    {
        assert(_window_handle == nullptr && "Winapi window should be closed before destruction" );
    }

    process::IProcess & WinapiWindow::getProcess()
    {
        return _process;
    }

    asio::awaitable<void> WinapiWindow::show()
    {
        co_await _async_context.ensureOnStrand();

        ShowWindowAsync(_window_handle, SW_SHOW);
        co_return;
    }

    asio::awaitable<void> WinapiWindow::hide()
    {
        co_await _async_context.ensureOnStrand();

        ShowWindowAsync(_window_handle, SW_HIDE);
        co_return;
    }

    native::window_handle WinapiWindow::getHandle() const
    {
        return _window_handle;
    }
    
    unsigned int WinapiWindow::getWidth() const
    {
        return _width;
    }
            
    unsigned int WinapiWindow::getHeight() const
    {
        return _height;
    }

    bool WinapiWindow::good() const
    {
        return _window_handle != nullptr;
    }
    
    asio::awaitable<void> WinapiWindow::close()
    {
        co_await _async_context.ensureOnStrand();

        if(_window_handle == nullptr)co_return;

        auto handle = _window_handle;
        _window_handle = nullptr;

        co_await _process.unregisterWindow(handle);
        co_return;
    }

    native::device_context WinapiWindow::acquireDeviceContext()
    {
        _acquired_device_context =  GetDC(_window_handle);
        spdlog::info(std::format("Device context {} acquired", *_acquired_device_context));
        return *_acquired_device_context;
    }

    bool WinapiWindow::releaseDeviceContext(native::device_context device_context)
    {
        if(device_context == NULL)
        {
            spdlog::error("Empty device context");
            return false;
        }

        if(!_acquired_device_context)
        {
            spdlog::warn("Device context not acquired, or already released");
            return false;
        }

        if(device_context != *_acquired_device_context)
        {
            spdlog::error("Invalid device context");
            return false;
        }

        if(ReleaseDC(_window_handle, device_context) == 0)
        {
            spdlog::error("Cannot release device context");
            return false;
        }
        
        _acquired_device_context = std::nullopt;
        spdlog::info(std::format("Device context {} released", device_context));

        return true;
    }
}