#pragma once

#include <functional>

#include "native/native.h"
#include <asio.hpp>

#include "process/process.hpp"
#include "window/window.hpp"
#include "render/render.hpp"
#include "ecs/ecs.hpp"
#include "asset/asset.hpp"

namespace astre::pipeline
{
    struct WindowAppState
    {
        process::IProcess & process;
        window::IWindow & window;
        render::IRenderer & renderer;
    };

    class WindowApp
    {
        public:
        WindowApp(std::string title, unsigned int width, unsigned int height)
            : _title(std::move(title)), _width(width), _height(height)
        {};

        template<class R, class F, class... Args>
        asio::awaitable<R> run(process::IProcess & process, F && fnc, Args && ... args)
        {
            window::Window window = co_await window::createWindow(process, _title, _width, _height);
            render::Renderer renderer = co_await render::createRenderer(*window);

            co_await process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{.context = async::AsyncContext(process.getExecutionContext()),
                .onDestroy = [&]() -> asio::awaitable<void>
                {
                    renderer->join();
                    co_await window->close();
                }
            });

            co_await window->show();

            co_return co_await fnc(WindowAppState{.process = process, .window = *window, .renderer = *renderer}, std::forward<Args>(args)...);
        }

        private:
            std::string _title;
            unsigned int _width;
            unsigned int _height;

    };

    struct GamePipelineState
    {
        ecs::Registry & registry;
        asset::ComponentLoaderRegistry & loader;
        asset::ComponentSerializerRegistry & serializer;
    };

    class GamePipeline
    {
        public:
            template<class R, class F, class... Args>
            asio::awaitable<R> run(F && fnc, Args && ... args)
            {
                ecs::Registry registry;
                asset::ComponentLoaderRegistry loader;
                asset::ComponentSerializerRegistry serializer;

                co_return co_await fnc(GamePipelineState{.registry = registry, .loader = loader, .serializer = serializer}, std::forward<Args>(args)...);
            }

    };
}