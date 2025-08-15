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

    world::WorldStreamer world_streamer;
    
    pipeline::LogicFrameTimer logic_timer;

    layout::DockSpace dock_space;

    panel::DrawContext ctx;

    panel::MainMenuBar main_menu_bar;
    panel::ScenePanel scene_panel;
    panel::PropertiesPanel properties_panel;
    panel::AssetsPanel assets_panel;
    panel::ViewportPanel viewport_panel;

    controller::SelectionOverlayController selection_overlay_controller;

    controller::EditorFlyCamera flycam;
};

asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
{
    // Load Vertex Buffers
    co_await asset::loadVertexBuffersPrefabs(app_state.renderer);
    
    // Load Shaders
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "debug_overlay");
    co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "picking_id64");

    // Load Scripts
    co_await asset::loadScript(app_state.script, paths.resources / "worlds" / "scripts" / "player_script.lua");

    ecs::Registry registry(app_state.process.getExecutionContext());

    asset::ResourceTracker resource_tracker(app_state.renderer, app_state.script,
        paths.resources / "shaders" / "glsl",
        paths.resources / "worlds" / "scripts"
    );

    // Create viewport FBO
    auto display_resources_res = co_await pipeline::buildDisplayResources(app_state.renderer, {1280, 728});
    if(!display_resources_res) 
    {
        spdlog::error("Failed to create display resources");
         co_return;
    }
    const auto & display_resources = *display_resources_res;
    // Get viewport FBO texture
    auto viewport_fbo_textures = app_state.renderer.getFrameBufferObjectTextures(display_resources.viewport_fbo);
    assert(viewport_fbo_textures.size() == 1);

    // Create picking FBO
    auto picking_resources_res = co_await pipeline::buildPickingResources(app_state.renderer, display_resources.size);
    if(!picking_resources_res)
    {
        spdlog::error("Failed to build picking resources");
        co_return;
    }
    const pipeline::PickingResources & picking_resources = *picking_resources_res;

    // Create deferred shading FBO
    auto render_resources_res = co_await pipeline::buildDeferredShadingResources(app_state.renderer, display_resources.size);
    if(!render_resources_res)
    {
        spdlog::error("Failed to build deferred shading resources");
        co_return;
    }
    const pipeline::DeferredShadingResources & render_resources = *render_resources_res;

    // Create orchestrator
    pipeline::PipelineOrchestrator<EditorFrame, EditorState, 5, 3> orchestrator(app_state.process,
        EditorState
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
            .world_streamer = world::WorldStreamer(  
                app_state.process.getExecutionContext(),
                asset::use_json,
                paths.resources / "worlds/levels/level_0.json",
                registry,
                resource_tracker,
                32.0f, 32
            ),

            .logic_timer = pipeline::LogicFrameTimer(),

            .dock_space = layout::DockSpace(),

            .ctx = panel::DrawContext{
                .viewport_texture = viewport_fbo_textures.at(0)
            },
            .main_menu_bar = panel::MainMenuBar(),
            .scene_panel = panel::ScenePanel(),
            .properties_panel = panel::PropertiesPanel(),
            .assets_panel = panel::AssetsPanel(resource_tracker),
            .viewport_panel = panel::ViewportPanel(),

            .selection_overlay_controller = controller::SelectionOverlayController(app_state.renderer),

            .flycam = controller::EditorFlyCamera()
        }
    );
    
    // Logic 0. summary start stage
    orchestrator.setLogicStage<0>(
        []
        (async::LifecycleToken & token, float, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            editor_state.logic_timer.start();
        }
    );

    // Logic 1. PRE ECS, load world depending on flycamera position
    orchestrator.setLogicStage<1>(
        []
        (async::LifecycleToken & token, float, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await pipeline::runPreECS(editor_state.app_state, editor_state.world_streamer, editor_state.flycam.getPosition());
        }
    );

    // Logic 2. ECS stage
    orchestrator.setLogicStage<2>(
        [&display_resources, &picking_resources]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await pipeline::runECS(editor_state.systems, dt, editor_frame.render_frame);

            editor_state.flycam.setViewportRect(editor_state.viewport_panel.getImgPos(), editor_state.viewport_panel.getImgSize());
            editor_state.flycam.setHovered(editor_state.viewport_panel.isHovered());

            co_await editor_state.flycam.update(dt, editor_state.app_state, picking_resources,  
                [&](ecs::Entity entity) { editor_state.scene_panel.selectEntity(entity); });

            editor_state.ctx.camera_position = editor_state.flycam.getPosition();

            // override camera matrices by editor camera
            editor_state.flycam.overrideFrame(editor_frame.render_frame, display_resources.aspect);
        }
    );

    // Logic 3. summary end stage
    orchestrator.setLogicStage<3>(
        []
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);

            editor_state.logic_timer.end();

            editor_state.ctx.logic_frame_time = editor_state.logic_timer.getFrameTime();
            editor_state.ctx.logic_fps = editor_state.logic_timer.getFPS();
        }
    );

    // Logic 4.
    bool should_reload_scene = true;
    orchestrator.setLogicStage<4>(
        [&should_reload_scene]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);

            if(should_reload_scene)
            {
                should_reload_scene = false;
                editor_state.scene_panel.loadEntitesDefs(editor_state.world_streamer);
            }

            // handle removed chunks
            for(const auto & removed_chunk : editor_state.scene_panel.getRemovedChunks())
            {
                editor_state.world_streamer.removeChunk(removed_chunk);
                should_reload_scene = true;
            }
            // handle created chunks
            for(const auto & new_chunk : editor_state.scene_panel.getCreatedChunks())
            {
                editor_state.world_streamer.writeChunk(new_chunk);
                should_reload_scene = true;
            }
            
            // handle added entities
            for(auto & [chunk_id, new_entities] : editor_state.scene_panel.getCreatedEntities())
            {
                for(auto & new_entity : new_entities)
                {
                    editor_state.world_streamer.createEntity(chunk_id, new_entity);
                    should_reload_scene = true;
                }
            }
            // handle updated entities
            for(auto & [chunk_id, updated_entities] : editor_state.scene_panel.getUpdatedEntities())
            {
                for(auto & updated_entity : updated_entities)
                {
                    editor_state.world_streamer.updateEntity(chunk_id, updated_entity);
                    should_reload_scene = true;
                }
            }
            // handle removed entities
            for(const auto & [chunk_id, removed_entities] : editor_state.scene_panel.getRemovedEntities())
            {
                for(const auto & removed_entity : removed_entities)
                {
                    editor_state.world_streamer.removeEntity(chunk_id, removed_entity);
                    should_reload_scene = true;
                }
            }

            editor_state.scene_panel.resetEvents();

            // handle selected entity
            const auto scene_selected_entity_def = editor_state.scene_panel.getSelectedEntityDef();

            if(editor_state.scene_panel.selectedEntityChanged())
            {
                editor_state.scene_panel.resetSelectedEntityChanged();
                editor_state.properties_panel.setSelectedEntityDef(scene_selected_entity_def);

                editor_state.selection_overlay_controller.update(scene_selected_entity_def, editor_frame.render_frame);
            }

            const auto properties_selected_entity_def = editor_state.properties_panel.getSelectedEntityDef();

            if(editor_state.properties_panel.propertiesChanged())
            {
                editor_state.properties_panel.resetPropertiesChanged();

                editor_state.selection_overlay_controller.update(properties_selected_entity_def, editor_frame.render_frame);

                // save to world
                if(properties_selected_entity_def)
                {
                    auto [chunk_id, entity_def] = *properties_selected_entity_def;

                    editor_state.world_streamer.updateEntity(chunk_id, entity_def);
                    should_reload_scene = true;
                }
            }

            // add overlay to frame
            editor_state.selection_overlay_controller.draw(editor_frame.render_frame);
        }
    );

    // Render 0.
    orchestrator.setRenderStage<0>(
        [&display_resources]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await editor_state.app_state.renderer.clearScreen({0.1f, 0.1f, 0.1f, 1.0f}, display_resources.viewport_fbo);
        }
    );

    // Render 1.
    orchestrator.setRenderStage<1>(
        [&render_resources, &picking_resources, &display_resources]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state) -> asio::awaitable<void>
        {
            co_stop_if(token);

            editor_state.ctx.stats = co_await pipeline::deferredShadingStage(
                editor_state.app_state.renderer,
                render_resources,
                alpha, prev, curr,
                display_resources.viewport_fbo);

            co_await pipeline::renderPickingIds(
                editor_state.app_state.renderer,
                curr,
                picking_resources
            ); 
        }
    );

    // Render 2.
    orchestrator.setRenderStage<2>(
        []
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state) -> asio::awaitable<void>
        {
            co_stop_if(token);
            
            co_await editor_state.app_state.gui.newFrame();

            co_await editor_state.app_state.gui.draw(&layout::DockSpace::build, &(editor_state.dock_space));
            
            editor_state.ctx.scene_dock_id = editor_state.dock_space.getLeftDockId();
            editor_state.ctx.properties_dock_id = editor_state.dock_space.getRightDockId();
            editor_state.ctx.assets_dock_id = editor_state.dock_space.getBottomDockId();
            editor_state.ctx.viewport_dock_id = editor_state.dock_space.getCenterDockId();

            co_await editor_state.app_state.gui.draw(&panel::MainMenuBar::draw, &editor_state.main_menu_bar, std::cref(editor_state.ctx));
            co_await editor_state.app_state.gui.draw(&panel::ScenePanel::draw, &editor_state.scene_panel, std::cref(editor_state.ctx));
            co_await editor_state.app_state.gui.draw(&panel::PropertiesPanel::draw, &editor_state.properties_panel, std::cref(editor_state.ctx));
            co_await editor_state.app_state.gui.draw(&panel::AssetsPanel::draw, &editor_state.assets_panel, std::cref(editor_state.ctx));
            co_await editor_state.app_state.gui.draw(&panel::ViewportPanel::draw, &editor_state.viewport_panel, std::cref(editor_state.ctx));

            co_await editor_state.app_state.gui.render();
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