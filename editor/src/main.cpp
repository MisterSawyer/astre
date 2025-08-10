#include "astre_editor.hpp"

using namespace astre;
using namespace astre::editor;

struct EditorFrame
{
    // per-frame logical state
    float delta;

    // per-frame rendering state
    render::Frame render_frame;
};

struct EditorState
{
    pipeline::AppState & app_state;

    ecs::Registry & registry;
    ecs::Systems systems;

    world::WorldStreamer & world_streamer;
};

asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
{
    co_await asset::loadVertexBuffersPrefabs(app_state.renderer);
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");

    script::ScriptRuntime script_runtime;
    co_await asset::loadScript(script_runtime, paths.resources / "worlds" / "scripts" / "player_script.lua");

    ecs::Registry registry(app_state.process.getExecutionContext());

    world::WorldStreamer world_streamer(  
        app_state.process.getExecutionContext(),
        paths.resources / "worlds/levels/level_0.json",
        registry,
        32.0f, 32);

    pipeline::FramesBuffer<EditorFrame> buffer;
    pipeline::PipelineOrchestrator<EditorFrame, EditorState, 6, 3> orchestrator(app_state.process, buffer,
        EditorState
        {
            .app_state = app_state,
            .registry = registry,
            .systems = ecs::Systems{
                .transform = ecs::system::TransformSystem(registry),
                .camera = ecs::system::CameraSystem( "camera", registry),
                .visual = ecs::system::VisualSystem(app_state.renderer, registry),
                .light = ecs::system::LightSystem(registry),
                .script = ecs::system::ScriptSystem(script_runtime, registry),
                .input = ecs::system::InputSystem(app_state.input, registry)
            },
            .world_streamer = world_streamer
        }
    );
    
    // Logic 0. summary start stage
    std::chrono::steady_clock::time_point logic_last_time;
    std::chrono::steady_clock::time_point logic_start;

    orchestrator.setLogicStage<0>(
        [&logic_last_time, &logic_start]
        (async::LifecycleToken & token, float, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            logic_last_time = logic_start;
            logic_start = std::chrono::steady_clock::now();
        }
    );

    // Logic 1. Input, World 
    orchestrator.setLogicStage<1>(
        []
        (async::LifecycleToken & token, float, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            auto & ex = editor_state.app_state.process.getExecutionContext();
            const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);
            // --- Input + world in parallel
            {
                auto g = asio::experimental::make_parallel_group(
                    asio::co_spawn(ex, editor_state.app_state.input.update(token),                    asio::deferred),
                    asio::co_spawn(ex, editor_state.world_streamer.updateLoadPosition({0.0f, 0.0f, 0.0f}),    asio::deferred)
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
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await pipeline::runECS(editor_state.systems, dt, editor_frame.render_frame);
        }
    );

    // Logic 3. summary end stage
    float logic_fps = 0.0f;
    float logic_frame_time = 0.0f;
    constexpr float logic_frame_smoothing = 0.9f;

    orchestrator.setLogicStage<3>(
        [&logic_start, &logic_frame_time, &logic_fps, &logic_last_time, &logic_frame_smoothing]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
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

    // Create viewport FBO
    auto viewport_fbo_res = co_await app_state.renderer.createFrameBufferObject(
        "fbo::viewport", {1280, 728},
        {
            {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
        }
    );
    if (!viewport_fbo_res) {
        spdlog::error("Failed to create viewport FBO");
        // TODO throw?
        co_return;
    }
    std::size_t viewport_fbo = *viewport_fbo_res;
    auto viewport_fbo_textures = app_state.renderer.getFrameBufferObjectTextures(viewport_fbo);
    assert(viewport_fbo_textures.size() == 1);

    panel::DrawContext ctx
    {
        .viewport_texture = viewport_fbo_textures.at(0)
    };

    panel::MainMenuBar main_menu_bar;
    panel::ScenePanel scene_panel(world_streamer);
    panel::PropertiesPanel properties_panel;
    panel::AssetsPanel assets_panel;
    panel::ViewportPanel viewport_panel;

    orchestrator.setLogicStage<4>(
        [&properties_panel, &scene_panel]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            if(scene_panel.selectedEntityChanged())
            {
                scene_panel.resetSelectedEntityChanged();
                properties_panel.setSelectedEntityDef(scene_panel.getSelectedEntityDef());
            }
        }
    );


    // save phase
    orchestrator.setLogicStage<5>(
        [&world_streamer, &properties_panel]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);

            if(properties_panel.propertiesChanged())
            {
                properties_panel.resetPropertiesChanged();
                // save to world
                const auto selected_entity_def = properties_panel.getSelectedEntityDef();
                if(selected_entity_def)
                {
                    const auto [chunk_id, entity_def] = *selected_entity_def;

                    world_streamer.updateEntity(chunk_id, entity_def, asset::use_json);
                }
            }
        }
    );



    orchestrator.setRenderStage<0>(
        [&app_state, &viewport_fbo]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await app_state.renderer.clearScreen({0.1f, 0.1f, 0.1f, 1.0f}, viewport_fbo);
        }
    );

    render::FrameStats render_stats;
    render::RenderOptions gbuffer_render_options
    {
        .mode = render::RenderMode::Solid
    };
    render::RenderOptions shadow_map_render_options
    {
        .mode = render::RenderMode::Solid,
        .polygon_offset = render::PolygonOffset{.factor = 1.5f, .units = 4.0f}
    };

    pipeline::DeferredShadingResources render_resources = co_await pipeline::buildDeferredShadingResources(app_state.renderer);
    orchestrator.setRenderStage<1>(
        [&app_state, &render_resources, &gbuffer_render_options, &shadow_map_render_options, &viewport_fbo, &render_stats]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr) -> asio::awaitable<void>
        {
            co_stop_if(token);

            render_stats = co_await pipeline::deferredShadingStage(
                app_state.renderer,
                render_resources,
                alpha, prev, curr,
                gbuffer_render_options, shadow_map_render_options,
                viewport_fbo);
        }
    );

    layout::DockSpace dock_space;

    orchestrator.setRenderStage<2>(
        [&app_state, 
        &dock_space,
        &ctx,
        &main_menu_bar,
        &scene_panel,
        &properties_panel,
        &assets_panel,
        &viewport_panel]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await app_state.gui.newFrame();

            co_await app_state.gui.draw(&layout::DockSpace::build, &dock_space);
            
            ctx.scene_dock_id = dock_space.getLeftDockId();
            ctx.properties_dock_id = dock_space.getRightDockId();
            ctx.assets_dock_id = dock_space.getBottomDockId();
            ctx.viewport_dock_id = dock_space.getCenterDockId();

            co_await app_state.gui.draw(&panel::MainMenuBar::draw, &main_menu_bar, std::cref(ctx));
            co_await app_state.gui.draw(&panel::ScenePanel::draw, &scene_panel, std::cref(ctx));
            co_await app_state.gui.draw(&panel::PropertiesPanel::draw, &properties_panel, std::cref(ctx));
            co_await app_state.gui.draw(&panel::AssetsPanel::draw, &assets_panel, std::cref(ctx));
            co_await app_state.gui.draw(&panel::ViewportPanel::draw, &viewport_panel, std::cref(ctx));

            co_await app_state.gui.render();
        }
    );


    // Sync stage
    orchestrator.setSyncStage([]
        (async::LifecycleToken & token, EditorState & state) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await state.app_state.renderer.present(); // Swap buffers
        }
    );

    co_await orchestrator.runLoop(token);

    co_return;
}

namespace astre::entry
{
    asio::awaitable<int> main(process::IProcess & process, const entry::AppPaths & paths)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("[main] external main started");
        
        spdlog::debug("[main] Starting pipeline::App");

        co_await pipeline::App{process, "astre editor", 1280, 720}.run(runMainLoop, paths);

        spdlog::debug("[main] pipeline::App ended");

        co_return 0;
    }
}