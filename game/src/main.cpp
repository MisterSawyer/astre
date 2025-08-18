#include <astre.hpp>
namespace astre::entry
{
    struct GameFrame
    {
        // per-frame logical state
        float delta;

        // per-frame rendering state
        render::Frame render_frame;
    };

    struct GameState
    {
        pipeline::AppState & app_state;

        ecs::Registry & registry;
        ecs::Systems systems;

        file::WorldStreamer world_streamer;
    };

    asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
    {
        co_await asset::loadVertexBuffersPrefabs(app_state.renderer);
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");

        co_await asset::loadScript(app_state.script, paths.resources / "worlds" / "scripts" / "player_script.lua");

        ecs::Registry registry(app_state.process.getExecutionContext());

        asset::ResourceTracker resource_tracker(app_state.renderer, app_state.script,
            paths.resources / "shaders",
            paths.resources / "worlds" / "scripts"
        );

        pipeline::PipelineOrchestrator<GameFrame, GameState, 4, 1> orchestrator(app_state.process,
            GameState
            {
                .app_state = app_state,
                .registry = registry,
                .systems = ecs::Systems{
                    .transform = ecs::system::TransformSystem(registry),
                    .camera = ecs::system::CameraSystem(5, registry),
                    .visual = ecs::system::VisualSystem(app_state.renderer, registry),
                    .light = ecs::system::LightSystem(registry),
                    .script = ecs::system::ScriptSystem(app_state.script, registry),
                    .input = ecs::system::InputSystem(app_state.input, registry)
                },
                .world_streamer = file::WorldStreamer(  
                    app_state.process.getExecutionContext(),
                    file::use_json,
                    paths.resources / "worlds/levels/level_0.json",
                    32.0f, 32)
            }
        );
        
        // Logic 0. summary start stage
        std::chrono::steady_clock::time_point logic_last_time;
        std::chrono::steady_clock::time_point logic_start;

        orchestrator.setLogicStage<0>(
            [&logic_last_time, &logic_start]
            (async::LifecycleToken & token, float, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);

                logic_last_time = logic_start;
                logic_start = std::chrono::steady_clock::now();
            }
        );

        // Logic 1. Input, World 
        orchestrator.setLogicStage<1>(
            []
            (async::LifecycleToken & token, float, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);

                auto & ex = game_state.app_state.process.getExecutionContext();
                const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

                // --- Input + world in parallel
                {
                    auto g = asio::experimental::make_parallel_group(
                        asio::co_spawn(ex, game_state.app_state.input.update(),                    asio::deferred),
                        asio::co_spawn(ex, game_state.world_streamer.updateLoadPosition({0.0f, 0.0f, 0.0f}),    asio::deferred)
                    );

                    auto [ord, e1, e2] =
                        co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

                    if (e1) std::rethrow_exception(e1);
                    if (e2) std::rethrow_exception(e2);
                }
            }
        );

        // Logic 2. ECS stage
        orchestrator.setLogicStage<2>(
            []
            (async::LifecycleToken & token, float dt, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await pipeline::runECS(game_state.systems, dt, game_frame.render_frame);
            }
        );

        // Logic 3. summary end stage
        float logic_fps = 0.0f;
        float logic_frame_time = 0.0f;
        constexpr float logic_frame_smoothing = 0.9f;

        orchestrator.setLogicStage<3>(
            [&logic_start, &logic_frame_time, &logic_fps, &logic_last_time, &logic_frame_smoothing]
            (async::LifecycleToken & token, float dt, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                const float frame_time_ms =
                    std::chrono::duration<float>(std::chrono::steady_clock::now() - logic_start).count() * 1000.0f;
                logic_frame_time = (logic_frame_time * logic_frame_smoothing) + (frame_time_ms * (1.0f - logic_frame_smoothing));

                const float current_fps =
                    1.0f / std::max(std::chrono::duration<float>(logic_start - logic_last_time).count(), 1e-6f);
                logic_fps = (logic_fps * logic_frame_smoothing) + (current_fps * (1.0f - logic_frame_smoothing));
            }
        );

        // Render 0. Deferred render stage
        const std::pair<unsigned, unsigned> viewport_size = {1280, 728};
        auto render_resources_res = co_await pipeline::buildDeferredShadingResources(app_state.renderer, viewport_size);
        if(!render_resources_res)
        {
            spdlog::error("Failed to build deferred shading resources");
            co_return;
        }
        const pipeline::DeferredShadingResources & render_resources = *render_resources_res;


        render::FrameStats render_stats;
        
        orchestrator.setRenderStage<0>(
            [&render_resources, &render_stats]
            (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & game_state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                auto interpolated_frame = render::interpolateFrame(prev, curr, alpha);

                render_stats = co_await pipeline::deferredShadingStage(
                        game_state.app_state.renderer,
                        render_resources,
                        interpolated_frame);
            }
        );
        
        // Sync stage
        orchestrator.setSyncStage([]
            (async::LifecycleToken & token, GameState& state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await state.app_state.renderer.present(); // Swap buffers
            }
        );

        co_await orchestrator.runLoop(token);
    }

    asio::awaitable<int> main(process::IProcess & process, const entry::AppPaths & paths)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("[main] external main started");
        
        spdlog::debug("[main] Starting pipeline::App");

        co_await pipeline::App{process, "astre game", 1280, 720}.run(runMainLoop, paths);

        spdlog::debug("[main] pipeline::App ended");

        co_return 0;
    }
}