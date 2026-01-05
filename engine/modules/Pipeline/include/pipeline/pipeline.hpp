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
#include "input/input.hpp"

#include "pipeline/app_state.hpp"
#include "pipeline/renderer_state.hpp"
#include "pipeline/frame_buffer.hpp"

#include "pipeline/display.hpp"
#include "pipeline/deferred_shading.hpp"
#include "pipeline/picking.hpp"
#include "pipeline/debug_overlay.hpp"

#include "pipeline/logic_frame_timer.hpp"
#include "pipeline/logic_pipelines.hpp"

namespace astre::pipeline
{    class App
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
            async::AsyncContext async_context(_process.getExecutionContext());

            window::Window window = co_await window::createWindow(_process, _title, _width, _height);
            // will spawn its own render thread
            // but getExecutionContext is a thread pool, so we never know on which thread it will spawn
            render::Renderer renderer = co_await render::createRenderer(*window);
            
            // it is stack local to coroutine - but this coroutine encapsulates the whole app
            async::LifecycleToken fnc_token;
            async::LifecycleToken app_token;

            input::InputService input(_process, fnc_token);

            gui::GUIService gui(_process, *window, *renderer);
            co_await gui.init();

            script::ScriptRuntime script_runtime;

            co_await _process.setWindowCallbacks(window->getHandle(), 
            process::WindowCallbacks{
                .onDestroy = [&async_context, &app_token, &fnc_token, &window, &renderer, &gui]() -> asio::awaitable<void>
                {
                    co_await async_context.ensureOnStrand();

                    // we should schedule closing of the fnc
                    fnc_token.requestStop();

                    // Wait until fnc coroutine exits
                    while (!fnc_token.isFinished()) {
                        co_await async_context.ensureOnStrand();
                    }

                    co_await gui.close();
                    
                    co_await async_context.ensureOnStrand();
                    renderer->join();
                    co_await window->close();

                    app_token.requestStop();
                    app_token.markFinished();
                },
                .onResize = [&fnc_token, &window, &renderer](unsigned int width, unsigned int height) -> asio::awaitable<void>
                {
                    co_stop_if(fnc_token);
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

            // Run fnc
            co_await std::invoke(
                std::forward<F>(fnc),
                fnc_token,
                AppState{
                    .process = _process,
                    .window = *window,
                    .renderer = *renderer,
                    .input = input,
                    .gui = gui,
                    .script = script_runtime
                },
                std::forward<Args>(args)...
            );
            
            fnc_token.markFinished();

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

    template<typename FrameState, typename LogicState, typename RenderState, std::size_t LogicStagesCount = 1, std::size_t RenderStagesCount = 1>
    class PipelineOrchestrator
    {
    public:
        using LogicStage = std::function<
            asio::awaitable<void>(async::LifecycleToken &, float, FrameState&, LogicState&)>;

        using RenderStage = std::function<
            asio::awaitable<void>(async::LifecycleToken &, float, const render::Frame &, const render::Frame &, LogicState &, RenderState &)>;

        using SyncStage = std::function<
            asio::awaitable<void>(async::LifecycleToken &, LogicState&)>;

        PipelineOrchestrator(process::IProcess & process, LogicState && logic_init_state, RenderState && render_init_state)
            :   _process(process),
                _buffer(),
                _logic_state(std::move(logic_init_state)),
                _render_state(std::move(render_init_state)),
                _accumulator(0.0f),
                _fixed_logic_step(1.0f / 10.0f)
        {}
        
        template<std::size_t Index>
        void setLogicStage(LogicStage stage)
        {
            static_assert(Index < LogicStagesCount, "Index out of range"); 
            _logic_stages.at(Index) = std::move(stage);
        }
        
        template<std::size_t Index>
        void setRenderStage(RenderStage stage)
        { 
            static_assert(Index < RenderStagesCount, "Index out of range"); 
            _render_stages.at(Index) = std::move(stage); 
        }

        void setSyncStage(SyncStage stage) 
        {
            _sync_stage = std::move(stage);
        }

        asio::awaitable<void> runLoop(async::LifecycleToken & token) 
        {    
            co_stop_if(token);

            if(_checkStagesAssignments() == false)
            {
                spdlog::error("[pipeline] Some stages are not assigned");
                co_return;
            }

            auto ex = co_await asio::this_coro::executor;

            std::chrono::steady_clock::time_point now;

            std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
            float alpha = 0.0f;

            while (token.stopRequested() == false) 
            {
                now = std::chrono::steady_clock::now();
                _accumulator += std::chrono::duration<float>(now - last_time).count();
                last_time = now;
                
                alpha = _accumulator / _fixed_logic_step;

                auto group = asio::experimental::make_parallel_group(
                    asio::co_spawn(ex, _runLogicStages(token), asio::deferred),
                    asio::co_spawn(ex, _runRenderStages(token, alpha), asio::deferred)
                );

                auto [order, e1, should_rotate, e2] = co_await group.async_wait(
                    asio::experimental::wait_for_all(),
                    asio::use_awaitable
                );

                if(e1)std::rethrow_exception(e1);
                if(e2)std::rethrow_exception(e2);
                    
                if (should_rotate)
                {
                    _buffer.rotate();
                }

                // sync
                if (_sync_stage) co_await _sync_stage(token, _logic_state);
            }
            
            co_return;
        }
    
    private:

        bool _checkStagesAssignments()
        {
            for(std::size_t logic_idx = 0; logic_idx < RenderStagesCount; ++logic_idx)
            {
                if(_logic_stages.at(logic_idx) == nullptr)
                {
                    spdlog::error("[pipeline] Logic stage {} is not set", logic_idx);
                    return false;
                }
            }

            for(std::size_t render_idx = 0; render_idx < RenderStagesCount; ++render_idx)
            {
                if(_render_stages.at(render_idx) == nullptr)
                {
                    spdlog::error("[pipeline] Render stage {} is not set", render_idx);
                    return false;
                }
            }

            if(_sync_stage == nullptr)
            {
                spdlog::error("[pipeline] Sync stage is not set");
                return false;
            }

            return true;
        }


        asio::awaitable<bool> _runLogicStages(async::LifecycleToken & token)
        {
            co_stop_if(token, false);

            auto ex = co_await asio::this_coro::executor;

            // we want to post those jobs asynchronously so we need to create suspension point
            // for the thread_pool to be able to pick up other jobs from the queue
            co_await asio::post(ex, asio::use_awaitable);

            bool should_rotate = false;

            while (_accumulator >= _fixed_logic_step)
            {
                should_rotate = true;
                for (std::size_t logic_idx = 0; logic_idx < LogicStagesCount; ++logic_idx)
                {
                    co_await _logic_stages.at(logic_idx)(token, _fixed_logic_step, _buffer.current(), _logic_state);
                }
                _accumulator -= _fixed_logic_step;
            }

            co_return should_rotate;  
        }

        asio::awaitable<void> _runRenderStages(async::LifecycleToken & token, float alpha)
        {
            co_stop_if(token);

            auto ex = co_await asio::this_coro::executor;

            // we want to post those jobs asynchronously so we need to create suspension point
            // for the thread_pool to be able to pick up other jobs from the queue
            co_await asio::post(ex, asio::use_awaitable);

            for (std::size_t render_idx = 0; render_idx < RenderStagesCount; ++render_idx)
            {
                co_await _render_stages.at(render_idx)(
                    token,
                    alpha, 
                    _buffer.beforePrevious().render_frame,
                    _buffer.previous().render_frame,
                    _logic_state,
                    _render_state);
            }
        }

        process::IProcess & _process;
        FramesBuffer<FrameState> _buffer;

        std::array<LogicStage, LogicStagesCount> _logic_stages;
        float _fixed_logic_step;
        float _accumulator;

        std::array<RenderStage, RenderStagesCount> _render_stages;

        SyncStage _sync_stage;

        LogicState _logic_state;

        RenderState _render_state;
    };
}