#include <atomic>

#include <astre.hpp>

namespace astre::entry
{
    class StatsWindow
    {
        public:
            void draw(float logic_fps, float logic_frame_time, const astre::render::FrameStats& stats,
                std::atomic<bool>* show_chunk_borders)
            {
                const ImGuiViewport* vp = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
                ImGui::SetNextWindowViewport(vp->ID);

                ImGuiWindowFlags flags =
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings;

                if (ImGui::Begin("StatsWindow", nullptr, flags))
                {
                    const ImGuiIO& io = ImGui::GetIO();
                    if (io.Framerate > 0.0f) {
                        ImGui::Text("Render FPS: %.1f", io.Framerate);
                        ImGui::Text("Render Frame Time: %.2f ms", 1000.0f / io.Framerate);
                    }
                    ImGui::Text("Logic FPS: %.1f", logic_fps);
                    ImGui::Text("Logic Frame Time: %.2f ms", logic_frame_time);
                    ImGui::Text("Draw Calls: %d", stats.draw_calls);
                    ImGui::Text("Vertices: %d", stats.vertices);
                    ImGui::Text("Triangles: %d", stats.triangles);

                    bool show = show_chunk_borders->load();
                    if (ImGui::Checkbox("Show chunk borders", &show))
                        show_chunk_borders->store(show);
                }

                ImGui::End();
            }
    };

    class CameraWindow
    {
        public:
            void draw(const astre::math::Vec3& position)
            {
                const ImGuiViewport* vp = ImGui::GetMainViewport();
                // pin to bottom-right corner
                const ImVec2 pos{vp->WorkPos.x + vp->WorkSize.x, vp->WorkPos.y + vp->WorkSize.y};
                ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
                ImGui::SetNextWindowViewport(vp->ID);

                ImGuiWindowFlags flags =
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings;

                if (ImGui::Begin("CameraWindow", nullptr, flags))
                {
                    ImGui::Text("Camera: %.2f, %.2f, %.2f", position.x, position.y, position.z);
                }

                ImGui::End();
            }
    };

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

        pipeline::LogicFrameTimer logic_timer;
        float logic_fps{0.0f};
        float logic_frame_time{0.0f};
    };

    struct GameRenderState
    {
        pipeline::RendererState & render_state;

        astre::render::FrameStats frame_stats;
    };


    asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
    {
        if(!co_await app_state.streamers.world_streamer.stream(paths.resources / "worlds" / "levels" / "level_0.json", file::use_json))
        {
            spdlog::error("Failed to open world file");
            co_return;
        }

        // stream (import -> cache) then load (cache -> runtime)
        const std::vector<std::filesystem::path> shader_files = {
            paths.resources / "shaders" / "glsl" / "deferred_shader",
            paths.resources / "shaders" / "glsl" / "deferred_lighting_pass",
            paths.resources / "shaders" / "glsl" / "shadow_depth",
            paths.resources / "shaders" / "glsl" / "basic_shader",
            paths.resources / "shaders" / "glsl" / "simple_NDC",
            paths.resources / "shaders" / "glsl" / "debug_overlay"
        };
        if(!co_await app_state.streamers.shader_streamer.stream(shader_files) ||
           !co_await app_state.loaders.shader_loader.sync(app_state.streamers.shader_streamer.keys(), app_state.streamers.shader_streamer))
        {
            spdlog::error("Failed to load shaders");
            co_return;
        }

        const std::vector<std::filesystem::path> script_files = {
            paths.resources / "worlds" / "scripts" / "player_script.lua", 
            paths.resources / "worlds" / "scripts" / "camera_script.lua"
        };
        if(!co_await app_state.streamers.script_streamer.stream(script_files) ||
           !co_await app_state.loaders.script_loader.sync(app_state.streamers.script_streamer.keys(), app_state.streamers.script_streamer))
        {
            spdlog::error("Failed to load scripts");
            co_return;
        }

        // basic prefabs already exists in memory, we only need to load them into renderer
        co_await app_state.loaders.mesh_loader.loadPrefabs();

        // construct render resources
        auto render_state_res = co_await pipeline::buildRendererState(app_state.renderer, {1280, 728});
        if(!render_state_res) 
        {
            spdlog::error("Failed to build renderer state");
            co_return;
        }

        // debug chunk-border overlay toggle (ImGui write / logic read -> atomic)
        std::atomic<bool> show_chunk_borders{false};
        
        // configure camera and load startup world chunks such that camera system can reference entity
        // and get its position in orchestrator logic on first frame
        constexpr ecs::Entity active_camera_entity = 5;
        const math::Vec3 startup_camera_position = app_state.streamers.world_streamer.findEntityPosition(active_camera_entity).value_or(math::Vec3{0.0f, 0.0f, 0.0f});
        co_await app_state.streamers.world_streamer.updateLoadPosition(startup_camera_position);
        if(!co_await app_state.loaders.chunk_loader.sync(app_state.streamers.world_streamer.keys(), app_state.streamers.world_streamer))
        {
            spdlog::error("Failed to load startup world chunks");
            co_return;
        }
        app_state.systems.camera.setActiveCameraEntity(active_camera_entity);

        // construct pipeline
        // with 4 logic stages and 3 render stages
        pipeline::PipelineOrchestrator<GameFrame, GameState, GameRenderState, 4, 3> orchestrator(app_state.process,
            GameState
            {
                .app_state = app_state,
                .logic_timer = pipeline::LogicFrameTimer(),
            },

            GameRenderState
            {
                .render_state = *render_state_res,
                .frame_stats = render::FrameStats{}
            }
        );
        
        // Logic 0. summary start stage
        orchestrator.setLogicStage<0>(
            []
            (async::LifecycleToken & token, float, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                game_state.logic_timer.start();
            }
        );

        // Logic 1. PRE ECS, load world depending on active camera position
        orchestrator.setLogicStage<1>(
            []
            (async::LifecycleToken & token, float, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await pipeline::runPreECS(
                    game_state.app_state,
                    game_state.app_state.streamers.world_streamer,
                    game_state.app_state.systems.camera.getActiveCameraPosition().value_or(math::Vec3{0.0f, 0.0f, 0.0f}));

                // Streamed chunks are a DefinitionSource like any asset cache, so
                // their entities upload through the same Stage 3 pipeline.
                if(!co_await game_state.app_state.loaders.chunk_loader.sync(game_state.app_state.streamers.world_streamer.keys(),
                    game_state.app_state.streamers.world_streamer))
                {
                    spdlog::error("Failed to load streamed world chunks");
                    co_return;
                }
            }
        );

        // Logic 2. ECS stage
        orchestrator.setLogicStage<2>(
            []
            (async::LifecycleToken & token, float dt, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await pipeline::runECS(game_state.app_state.systems, dt, game_frame.render_frame);
            }
        );

        // Logic 3. summary end stage
        orchestrator.setLogicStage<3>(
            []
            (async::LifecycleToken & token, float dt, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                game_state.logic_timer.end();

                game_state.logic_frame_time = game_state.logic_timer.getFrameTime();
                game_state.logic_fps = game_state.logic_timer.getFPS();
            }
        );

        // Render 0.
        orchestrator.setRenderStage<0>(
            []
            (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & game_state, GameRenderState & game_renderer_state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                // clear screen
                co_await game_state.app_state.renderer.clearScreen({0.1f, 0.1f, 0.1f, 1.0f});
            }
        );
        
        // Render 1.
        orchestrator.setRenderStage<1>(
            [&show_chunk_borders]
            (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & game_state, GameRenderState & game_renderer_state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                auto interpolated_frame = render::interpolateFrame(prev, curr, alpha);
                // calculate light space matrices on interpolated frame
                interpolated_frame.light_space_matrices = ecs::system::calculateLightSpaceMatrices(interpolated_frame);

                //render deferred shading directly to screen
                game_renderer_state.frame_stats = co_await pipeline::deferredShadingStage(
                        game_state.app_state.renderer,
                        game_renderer_state.render_state.deferred_shading,
                        interpolated_frame);

                // chunk-border debug overlay directly to screen, state by state
                if(show_chunk_borders.load())
                    co_await pipeline::renderChunkBorders(
                        game_state.app_state.renderer,
                        game_renderer_state.render_state.debug_overlay,
                        game_state.app_state.streamers.world_streamer,
                        interpolated_frame.view_matrix,
                        interpolated_frame.proj_matrix);
            }
        );
        
        // Render 2. render GUI
        StatsWindow stats_window;
        CameraWindow camera_window;
        orchestrator.setRenderStage<2>(
            [&stats_window, &camera_window, &show_chunk_borders]
            (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & game_state, GameRenderState & game_renderer_state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await game_state.app_state.gui.newFrame();
                co_await game_state.app_state.gui.draw(&StatsWindow::draw, &stats_window,
                    game_state.logic_fps,
                    game_state.logic_frame_time,
                    std::cref(game_renderer_state.frame_stats),
                    &show_chunk_borders);

                co_await game_state.app_state.gui.draw(&CameraWindow::draw, &camera_window,
                    std::cref(curr.camera_position));

                co_await game_state.app_state.gui.render();
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
