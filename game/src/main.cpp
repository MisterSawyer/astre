#include <astre.hpp>

namespace astre::entry
{
    class StatsWindow
    {
        public:
            void draw(float logic_fps, float logic_frame_time, const astre::render::FrameStats& stats)
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

        ecs::Registry & registry;
        ecs::Systems systems;

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
        ecs::Registry registry(app_state.process.getExecutionContext());

        // Filesystem streaming
        file::MeshStreamer mesh_streamer(
            app_state.process.getExecutionContext(),
            paths.resources / "meshes"
        );

        file::ShaderStreamer shader_streamer(
            app_state.process.getExecutionContext(),
            paths.resources / "shaders" / "glsl"
        );

        file::WorldStreamer world_streamer(  
            app_state.process.getExecutionContext(),
            paths.resources / "worlds" / "levels" / "level_0.json",
            file::use_json,
            32.0f, 32
        );

        file::ScriptStreamer script_streamer(
            app_state.process.getExecutionContext(),
            paths.resources / "worlds" / "scripts"
        );

        // Module loaders
        loader::MeshLoader mesh_loader(app_state.renderer);
        loader::ShaderLoader shader_loader(app_state.renderer);
        loader::ScriptLoader script_loader(app_state.script);
        loader::EntityLoader entity_loader(registry);

        // load resources from files into memory
        co_await shader_streamer.load({"deferred_shader", "deferred_lighting_pass", "shadow_depth", "basic_shader", "simple_NDC", "debug_overlay"});
        co_await script_streamer.load({std::filesystem::path("player_script.lua"), std::filesystem::path("camera_script.lua")});


        // basic prefabs already exists in memory, we only need to load them into renderer
        co_await mesh_loader.loadPrefabs();

        // load shaders from memory to renderer using shader loader
        co_await loader::loadStreamedResources({"deferred_shader", "deferred_lighting_pass", "shadow_depth", "basic_shader", "simple_NDC", "debug_overlay"}, 
            shader_streamer, shader_loader);

        // load scripts from memory to ScriptRuntime using script loader
        co_await loader::loadStreamedResources({"player_script", "camera_script"}, script_streamer, script_loader);


        // construct render resources
        auto render_state_res = co_await pipeline::buildRendererState(app_state.renderer, {1280, 728});
        if(!render_state_res) 
        {
            spdlog::error("Failed to build renderer state");
            co_return;
        }

        // construct pipeline
        // with 4 logic stages and 3 render stages
        pipeline::PipelineOrchestrator<GameFrame, GameState, GameRenderState, 4, 3> orchestrator(app_state.process,
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

        // Logic 1. PRE ECS, load world depending on flycamera position
        orchestrator.setLogicStage<1>(
            [&world_streamer, &entity_loader]
            (async::LifecycleToken & token, float, GameFrame & game_frame, GameState & game_state)  -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await pipeline::runPreECS(game_state.app_state, world_streamer, {0.0f, 0.0f, 0.0f});

                // Pre ECS stage will load appropriate chunks into memory using world streamer
                for(const auto & chunk_id : world_streamer.getAllChunks())
                {
                    auto chunk = world_streamer.read(chunk_id);
                    if (chunk == nullptr)continue;
                
                    // then load every entity in chunk
                    for(const auto & entity_def : chunk->entities())
                    {
                        co_await entity_loader.load(entity_def);
                    }
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
            []
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
            }
        );
        
        // Render 2. render GUI
        StatsWindow stats_window;
        orchestrator.setRenderStage<2>(
            [&stats_window]
            (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & game_state, GameRenderState & game_renderer_state) -> asio::awaitable<void>
            {
                co_stop_if(token);
                co_await game_state.app_state.gui.newFrame();
                co_await game_state.app_state.gui.draw(&StatsWindow::draw, &stats_window,
                    game_state.logic_fps,
                    game_state.logic_frame_time,
                    std::cref(game_renderer_state.frame_stats));

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
