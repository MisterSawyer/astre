#include <astre.hpp>

namespace astre::entry
{
    asio::awaitable<int> main(process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");
        
        window::Window window = co_await window::createWindow(process, "astre", 1280, 720);
        render::Renderer renderer = co_await render::createRenderer(*window);

        co_await process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{.context = async::AsyncContext(process.getExecutionContext()),
                .onDestroy = [&]() -> asio::awaitable<void>
                {
                    spdlog::info("Window callback onDestroy");
                    co_await renderer->close();
                    co_await window->close();
                }
            });
        co_await window->show();


        async::AsyncContext main_loop_context(process.getExecutionContext());
        while(window->good())
        {
            co_await main_loop_context.ensureOnStrand();

            co_await renderer->clearScreen({1.0f, 1.0f, 0.0f, 1.0f});
            co_await renderer->present();
            std::this_thread::sleep_for(100ms);
        };

        co_return 0;
    }
}