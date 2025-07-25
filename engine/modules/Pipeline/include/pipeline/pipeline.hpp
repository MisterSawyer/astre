#pragma once

#include <functional>

#include "native/native.h"
#include <asio.hpp>

#include "process/process.hpp"
#include "window/window.hpp"
#include "render/render.hpp"
#include "ecs/ecs.hpp"
#include "asset/asset.hpp"
#include "input/input.hpp"

namespace astre::pipeline
{
    struct WindowAppState
    {
        process::IProcess & process;
        window::IWindow & window;
        render::IRenderer & renderer;

        input::InputService & input_service;
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

            // input system
            async::AsyncContext input_context(process.getExecutionContext());
            input::InputService input_service(input_context);

            co_await process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{.context = input_context,
                .onDestroy = [&window, &renderer]() -> asio::awaitable<void>
                {
                    renderer->join();
                    co_await window->close();
                },
                .onResize = [&window, &renderer](unsigned int width, unsigned int height) -> asio::awaitable<void>
                {
                    //co_await window->resize(width, height); is it even needed?
                    co_await renderer->updateViewportSize(width, height);
                },
                .onKeyPress = [&input_service](int key) -> asio::awaitable<void>
                {
                    co_await input_service.recordKeyPressed(input::keyToInputCode(key));
                },
                .onKeyRelease = [&input_service](int key) -> asio::awaitable<void>
                {
                    co_await input_service.recordKeyReleased(input::keyToInputCode(key));
                },
                .onMouseButtonDown = [&input_service](int key) -> asio::awaitable<void>
                {
                    co_await input_service.recordKeyPressed(input::keyToInputCode(key));
                },
                .onMouseButtonUp = [&input_service](int key) -> asio::awaitable<void>
                {
                    co_await input_service.recordKeyReleased(input::keyToInputCode(key));
                },
                .onMouseMove = [&input_service](int x, int y, float dx, float dy) -> asio::awaitable<void>
                {
                    co_await input_service.recordMouseMoved((float)x, (float)y, dx, dy);
                }
            });

            co_await window->show();

            co_return co_await fnc(
                WindowAppState
                {
                    .process = process,
                    .window = *window,
                    .renderer = *renderer,
                    .input_service = input_service
                },
                std::forward<Args>(args)...);
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

                // create systems

                co_return co_await fnc(
                    GamePipelineState
                    {
                        .registry = registry,
                        .loader = loader,
                        .serializer = serializer
                    },
                    std::forward<Args>(args)...);
            }
    };
}