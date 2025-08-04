#pragma once

#include <array>
#include <functional>
#include <atomic>

#include "native/native.h"
#include <asio.hpp>

#include "async/async.hpp"
#include "process/process.hpp"
#include "window/window.hpp"
#include "render/render.hpp"
#include "ecs/ecs.hpp"
#include "asset/asset.hpp"
#include "input/input.hpp"

namespace astre::pipeline
{
    struct AppState
    {
        async::LifecycleToken & lifecycle;

        process::IProcess & process;
        window::IWindow & window;
        render::IRenderer & renderer;
        input::InputService & input;
    };

    class App
    {
        public:
        App(process::IProcess & process, std::string title, unsigned int width, unsigned int height)
                :   _process(process),
                    _title(std::move(title)),
                    _width(width), _height(height)
        {};

        template<class F, class... Args>
        asio::awaitable<void> run(F && fnc, Args && ... args)
        {
            window::Window window = co_await window::createWindow(_process, _title, _width, _height);
            // will spawn its own render thread
            // but getExecutionContext is a thread pool, so we never know on which thread it will spawn
            render::Renderer renderer = co_await render::createRenderer(*window);
            
            // it is stack local to coroutine - but this coroutine encapsulates the whole app
            async::LifecycleToken fnc_token;
            async::LifecycleToken app_token;

            input::InputService input(fnc_token, _process);

            co_await _process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{
                .onDestroy = [this, &app_token, &fnc_token, &window, &renderer, &input]() -> asio::awaitable<void>
                {
                    // we should schedule closing of the fnc
                    fnc_token.requestStop();

                    // Wait until fnc coroutine exits
                    while (!fnc_token.isFinished()) {
                        co_await asio::post(_process.getExecutionContext(), asio::use_awaitable);
                    }

                    renderer->join();
                    co_await window->close();

                    app_token.requestStop();
                    app_token.markFinished();
                },
                .onResize = [&window, &renderer](unsigned int width, unsigned int height) -> asio::awaitable<void>
                {
                    //co_await window->resize(width, height); is it even needed?
                    co_await renderer->updateViewportSize(width, height);
                },
                .onKeyPress = [&input](int key) -> asio::awaitable<void>
                {
                    co_await input.recordKeyPressed(input::keyToInputCode(key));
                },
                .onKeyRelease = [&input](int key) -> asio::awaitable<void>
                {
                    co_await input.recordKeyReleased(input::keyToInputCode(key));
                },
                .onMouseButtonDown = [&input](int key) -> asio::awaitable<void>
                {
                    co_await input.recordKeyPressed(input::keyToInputCode(key));
                },
                .onMouseButtonUp = [&input](int key) -> asio::awaitable<void>
                {
                    co_await input.recordKeyReleased(input::keyToInputCode(key));
                },
                .onMouseMove = [&input](int x, int y, float dx, float dy) -> asio::awaitable<void>
                {
                    co_await input.recordMouseMoved((float)x, (float)y, dx, dy);
                }
            });

            co_await window->show();

            auto cancel_slot = fnc_token.getSlot();
            // Run fnc in its own coroutine
            co_await asio::co_spawn(
                _process.getExecutionContext(),
                fnc(
                    AppState{
                        .lifecycle = fnc_token,
                        .process = _process,
                        .window = *window,
                        .renderer = *renderer,
                        .input = input
                    },
                    std::forward<Args>(args)...
                ),
                asio::bind_cancellation_slot(cancel_slot, asio::use_awaitable)
            );
            
            // Wait for app exit
            while (!app_token.isFinished()) {
                co_await asio::post(_process.getExecutionContext(), asio::use_awaitable);
            }

            spdlog::debug("[pipeline] App ended");
        }

        private:
            process::IProcess & _process;

            std::string _title;
            unsigned int _width;
            unsigned int _height;
    };


    template <typename T, std::size_t Count = 3>
    class FramesBuffer : public std::array<T, Count>
    {
    public:        
        static_assert(Count >= 2, "Need at least 2 frames for interpolation");

        T& current()                    { return std::array<T, Count>::at(_index); }
        const T& previous() const       { return std::array<T, Count>::at((_index + Count - 1) % Count); }
        const T& beforePrevious() const { return std::array<T, Count>::at((_index + Count - 2) % Count); }

        void rotate() { _index = (_index + 1) % Count; }
        std::size_t index() const { return _index; }

    private:
        std::size_t _index = 0;
    };

    template<typename FrameState, typename StageState, std::size_t LogicSubstagesCount = 1>
    class PipelineOrchestrator
    {
    public:
        using LogicSubStage = std::function<
            asio::awaitable<void>(float, FrameState&, StageState&)>;
        using RenderStage = std::function<
            asio::awaitable<void>(float, const render::Frame &, const render::Frame &, StageState&)>;
        using SyncStage = std::function<
            asio::awaitable<void>(StageState&)>;

        PipelineOrchestrator(process::IProcess & process, FramesBuffer<FrameState>& buffer, StageState && init_state)
            :   _process(process),
                _buffer(buffer),
                _stage_state(std::move(init_state)),
                _accumulator(0.0f),
                _fixed_logic_step(1.0f / 10.0f)
        {}
        
        template<std::size_t Index>
        void setLogicSubstage(LogicSubStage stage)
        {
            static_assert(Index < LogicSubstagesCount, "Index out of range"); 
            _logic_substages.at(Index) = (std::move(stage));
        }
        void setRenderStage(RenderStage stage) { _render = std::move(stage); }
        void setPostSync(SyncStage sync)      { _post_sync = std::move(sync); }

        asio::awaitable<void> runLoop(async::LifecycleToken & token) 
        {        
            std::chrono::steady_clock::time_point now;

            std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
            
            asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

            while (cs.cancelled() == asio::cancellation_type::none) 
            {
                now = std::chrono::steady_clock::now();
                _accumulator += std::chrono::duration<float>(now - last_time).count();
                last_time = now;

                if(_accumulator >= _fixed_logic_step)
                {
                    _buffer.rotate();
                }

                if(cs.cancelled() != asio::cancellation_type::none)
                {
                    spdlog::debug("[pipeline] Pipeline cancelled");
                    token.markFinished();
                    co_return;
                }
                try
                {
                    const render::Frame & render_curr = _buffer.previous().render_frame;
                    const render::Frame & render_prev = _buffer.beforePrevious().render_frame;
                    float alpha = _accumulator / _fixed_logic_step;
                    co_await (_runFixedLogic(token) && 
                        _render(alpha, render_prev, render_curr, _stage_state));
                }
                catch (const asio::system_error& e)
                {
                    if (e.code() == asio::error::operation_aborted) {
                        spdlog::debug("[pipeline] Pipeline cancelled");
                        co_return;
                    }
                    throw;
                }

                // sync
                if(cs.cancelled() != asio::cancellation_type::none)
                {
                    spdlog::debug("[pipeline] Pipeline cancelled");
                    token.markFinished();
                    co_return;
                }
                if (_post_sync) co_await _post_sync(_stage_state);
            }
            token.markFinished();
            co_return;
        }
    
    private:

        asio::awaitable<void> _runFixedLogic(async::LifecycleToken & token)
        {
            asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
            while (_accumulator >= _fixed_logic_step)
            {
                for (std::size_t logic_idx = 0; logic_idx < LogicSubstagesCount; ++logic_idx)
                {
                    if (cs.cancelled() != asio::cancellation_type::none)
                    {
                        spdlog::debug("[pipeline] Logic cancelled");
                        token.markFinished();
                        co_return;
                    }
                    co_await _logic_substages.at(logic_idx)(_fixed_logic_step, _buffer.current(), _stage_state);
                }
                _accumulator -= _fixed_logic_step;
            }
            co_return;  
        }

        process::IProcess & _process;
        FramesBuffer<FrameState> & _buffer;

        std::array<LogicSubStage, LogicSubstagesCount> _logic_substages;
        float _fixed_logic_step;
        float _accumulator;

        RenderStage _render;
        SyncStage _post_sync;
        StageState _stage_state;
    };

    // const and where
    // are valid during all frames 
    struct RenderResources
    {
        std::size_t deferred_fbo; // where
        std::size_t shadow_map_shader; // const
        std::vector<std::size_t> deferred_textures; // where

        std::vector<std::size_t> shadow_map_fbos; // where
        std::vector<std::size_t> shadow_map_textures; // where

        std::size_t screen_quad_vb; // where
        std::size_t screen_quad_shader; // const
    };

    asio::awaitable<void> renderFrameToGBuffer(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources);

    asio::awaitable<void> renderFrameToShadowMaps(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources);
    
    asio::awaitable<void> renderGBufferToScreen(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources);

}