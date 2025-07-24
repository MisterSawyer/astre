#include <astre.hpp>

namespace astre::entry
{
    asio::awaitable<int> main(process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");
        
        window::Window window = co_await window::createWindow(process, "astre", 1280, 720);

        co_await window->show();

        // connect input system to window and setup system objects callbacks
        co_await process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{.context = async::AsyncContext(process.getExecutionContext()),
                // process notifies us when the window is destroyed
                .onDestroy = [&window]() -> asio::awaitable<void>
                {
                    spdlog::info("Window callback onDestroy");
                    co_await window->close(); // and destroy window object
                }
            });

        async::AsyncContext input_context(process.getExecutionContext());

        co_await input_context.ensureOnStrand();

        while(window->good())
        {
            std::this_thread::sleep_for(100ms);
        };

        co_return 0;
    }
}