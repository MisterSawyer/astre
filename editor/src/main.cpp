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
    
    pipeline::LogicFrameTimer logic_timer;

    layout::DockSpace dock_space;

    model::DrawContext ctx;

    panel::MainMenuBar main_menu_bar;
    panel::ScenePanel scene_panel;
    panel::PropertiesPanel properties_panel;
    panel::AssetsPanel assets_panel;
    panel::ViewportPanel viewport_panel;

    controller::SelectionOverlayController selection_overlay_controller;
    controller::TranslateOverlayController translate_overlay_controller;

    controller::EditorFlyCamera flycam;
    controller::ViewportEntityPicker viewport_entity_picker;
};

struct EditorRenderState
{
    const pipeline::RendererState & render_state;
    const pipeline::PickingResources & picking_resources;
};

asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
{
    // ECS registry
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
    co_await shader_streamer.load({"deferred_shader", "deferred_lighting_pass", "shadow_depth", "basic_shader", "simple_NDC", "debug_overlay", "picking_id64"});
    co_await script_streamer.load({std::filesystem::path("player_script.lua"), std::filesystem::path("camera_script.lua")});
    
    // basic prefabs already exists in memory, we only need to load them into renderer
    co_await mesh_loader.loadPrefabs();

    // load shaders from memory to Renderer using shader loader
    co_await loader::loadStreamedResources({"deferred_shader", "deferred_lighting_pass", "shadow_depth", "basic_shader", "simple_NDC", "debug_overlay", "picking_id64"},
        shader_streamer, shader_loader);

    // load scripts from memory to ScriptRuntime using script loader
    co_await loader::loadStreamedResources({"player_script", "camera_script"}, script_streamer, script_loader);


    model::WorldSnapshot world_snapshot;

    // render resources
    auto render_state_res = co_await pipeline::buildRendererState(app_state.renderer, {1280, 728});
    if(!render_state_res) 
    {
        spdlog::error("Failed to build renderer state");
        co_return;
    }

    // Create picking FBO
    auto picking_resources_res = co_await pipeline::buildPickingResources(app_state.renderer, render_state_res->display.size);
    if(!picking_resources_res)
    {
        spdlog::error("Failed to build picking resources");
        co_return;
    }

    // Create orchestrator
    pipeline::PipelineOrchestrator<EditorFrame, EditorState, EditorRenderState, 5, 3> orchestrator(app_state.process,
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

            .logic_timer = pipeline::LogicFrameTimer(),

            .dock_space = layout::DockSpace(),

            .ctx = model::DrawContext{
                .viewport_texture = render_state_res->viewport_fbo_textures.at(0)
            },
            .main_menu_bar = panel::MainMenuBar(),
            .scene_panel = panel::ScenePanel(world_snapshot),
            .properties_panel = panel::PropertiesPanel(),
            .assets_panel = panel::AssetsPanel(),
            .viewport_panel = panel::ViewportPanel(),

            .selection_overlay_controller = controller::SelectionOverlayController(app_state.renderer),
            .translate_overlay_controller = controller::TranslateOverlayController(app_state.renderer),
            
            .flycam = controller::EditorFlyCamera(),

            .viewport_entity_picker = controller::ViewportEntityPicker( 
                    app_state.renderer,
                    app_state.input,
                    *picking_resources_res,
                    world_snapshot)
        },

        EditorRenderState
        {
            .render_state = *render_state_res,
            .picking_resources = *picking_resources_res
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
        [&world_streamer]
        (async::LifecycleToken & token, float, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await pipeline::runPreECS(editor_state.app_state, world_streamer, editor_state.flycam.getPosition());
        }
    );

    // Logic 2. ECS stage
    orchestrator.setLogicStage<2>(
        [&render_state_res]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await pipeline::runECS(editor_state.systems, dt, editor_frame.render_frame);

            editor_state.viewport_entity_picker.setViewportRect(
                editor_state.viewport_panel.getImgPos(),
                editor_state.viewport_panel.getImgSize());
            editor_state.viewport_entity_picker.setHovered(editor_state.viewport_panel.isHovered());

            editor_state.flycam.setViewportRect(    
                editor_state.viewport_panel.getImgPos(),
                editor_state.viewport_panel.getImgSize());
            editor_state.flycam.setHovered(editor_state.viewport_panel.isHovered());

            editor_state.translate_overlay_controller.setViewportRect(
                    editor_state.viewport_panel.getImgPos(),
                    editor_state.viewport_panel.getImgSize());
            editor_state.translate_overlay_controller.setHovered(editor_state.viewport_panel.isHovered());

            co_await editor_state.flycam.update(dt, editor_state.app_state.process, editor_state.app_state.input);

            editor_state.ctx.camera_position = editor_state.flycam.getPosition();

            // override camera matrices by editor camera
            editor_state.flycam.overrideFrame(editor_frame.render_frame, render_state_res->display.aspect);
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
    bool should_reload_world = true;
    orchestrator.setLogicStage<4>(
        [&should_reload_world, &world_snapshot, &world_streamer, &entity_loader]
        (async::LifecycleToken & token, float dt, EditorFrame & editor_frame, EditorState & editor_state)  -> asio::awaitable<void>
        {
            co_stop_if(token);

            if(should_reload_world)
            {
                should_reload_world = false;
                world_snapshot.load(world_streamer);

                for(const auto & [chunk_id, entity_map] : world_snapshot.mapping)
                {
                    for(const auto & [entity_id, entity_def] : entity_map)
                    {
                        co_await entity_loader.load(entity_def);
                    }
                }
            }

            // handle removed chunks
            // for(const auto & removed_chunk : editor_state.scene_panel.getRemovedChunks())
            // {
            //     editor_state.world_streamer.removeChunk(removed_chunk);
            //     should_reload_scene = true;
            // }
            // handle created chunks
            // for(const auto & new_chunk : editor_state.scene_panel.getCreatedChunks())
            // {
            //     editor_state.world_streamer.writeChunk(new_chunk);
            //     should_reload_scene = true;
            // }
            
            // handle added entities
            // for(auto & [chunk_id, new_entities] : editor_state.scene_panel.getCreatedEntities())
            // {
            //     for(auto & new_entity : new_entities)
            //     {
            //         //editor_state.world_streamer.createEntity(chunk_id, new_entity);
            //         should_reload_scene = true;
            //     }
            // }
            // handle updated entities
            // for(auto & [chunk_id, updated_entities] : editor_state.scene_panel.getUpdatedEntities())
            // {
            //     for(auto & updated_entity : updated_entities)
            //     {
            //         //editor_state.world_streamer.updateEntity(chunk_id, updated_entity);
            //         should_reload_scene = true;
            //     }
            // }
            // handle removed entities
            // for(const auto & [chunk_id, removed_entities] : editor_state.scene_panel.getRemovedEntities())
            // {
            //     for(const auto & removed_entity : removed_entities)
            //     {
            //         editor_state.world_streamer.removeEntity(chunk_id, removed_entity);
            //         should_reload_scene = true;
            //     }
            // }

            //editor_state.scene_panel.resetEvents();

            editor_state.scene_panel.updateSelectedEntity(editor_state.ctx.selection_controller);
            editor_state.properties_panel.updateSelectedEntity(editor_state.ctx.selection_controller);

            editor_state.selection_overlay_controller.update(editor_state.ctx, editor_frame.render_frame);
            editor_state.translate_overlay_controller.update(editor_state.ctx, editor_state.app_state.input, editor_frame.render_frame);

            if(!editor_state.translate_overlay_controller.isDragging()){
                co_await editor_state.viewport_entity_picker.updateSelectedEntity(editor_state.ctx.selection_controller);
            }
            else
            {
                auto p = editor_state.translate_overlay_controller.takeDraggedPosition();
                if(p && editor_state.ctx.selection_controller.isAnyEntitySelected())
                {
                    const auto serialized_pos = math::serialize(*p);

                    proto::ecs::EntityDefinition pending_def = editor_state.ctx.selection_controller.getEntitySelection();

                    pending_def.mutable_transform()->mutable_position()->CopyFrom(serialized_pos);

                    editor_state.ctx.selection_controller.updateSelectedEntity(pending_def);
                }
            }

            if(
                editor_state.ctx.selection_controller.isAnyChunkSelected() &&
                editor_state.ctx.selection_controller.isAnyEntitySelected() &&
                editor_state.ctx.selection_controller.selectedEntityUpdated())
            {
                editor_state.ctx.selection_controller.clearSelectedEntityUpdate();

                // save to world
                // editor_state.world_streamer.updateEntity(   
                //     editor_state.ctx.selection_controller.getChunkSelection(),
                //     editor_state.ctx.selection_controller.getEntitySelection()
                // );
            }

            // add selection overlay to frame
            editor_state.selection_overlay_controller.draw(editor_frame.render_frame);

            // add translate overlay to frame
            editor_state.translate_overlay_controller.draw(editor_frame.render_frame);
        }
    );

    // Render 0.
    orchestrator.setRenderStage<0>(
        []
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state, EditorRenderState & editor_render_state) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await editor_state.app_state.renderer.clearScreen({0.5f, 0.1f, 0.1f, 1.0f}, editor_render_state.render_state.display.viewport_fbo);
        }
    );

    // Render 1.
    orchestrator.setRenderStage<1>(
        []
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state, EditorRenderState & editor_render_state) -> asio::awaitable<void>
        {
            co_stop_if(token);

            // Render interpolated frame using prev <-> curr, using alpha
            auto interpolated_frame = render::interpolateFrame(prev, curr, alpha);
            // calculate light space matrices on interpolated frame
            interpolated_frame.light_space_matrices = ecs::system::calculateLightSpaceMatrices(interpolated_frame);

            editor_state.ctx.stats = co_await pipeline::deferredShadingStage(
                editor_state.app_state.renderer,
                editor_render_state.render_state.deferred_shading,
                interpolated_frame,
                editor_render_state.render_state.display.viewport_fbo);

            co_await pipeline::renderDebugOverlay(
                editor_state.app_state.renderer,
                editor_render_state.render_state.debug_overlay,
                interpolated_frame,
                editor_render_state.render_state.display.viewport_fbo
            );

            co_await pipeline::renderPickingIds(
                editor_state.app_state.renderer,
                editor_render_state.picking_resources,
                interpolated_frame
            ); 
        }
    );

    // Render 2.
    orchestrator.setRenderStage<2>(
        []
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, EditorState & editor_state, EditorRenderState & editor_render_state) -> asio::awaitable<void>
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