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
};

asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
{

    pipeline::FramesBuffer<EditorFrame> buffer;
    pipeline::PipelineOrchestrator<EditorFrame, EditorState, 1, 2> orchestrator(app_state.process, buffer,
        EditorState
        {
            .app_state = app_state
        }
    );
    
    orchestrator.setLogicStage<0>(
        []
        (astre::async::LifecycleToken & token, float dt, EditorFrame & frame, EditorState & state) -> asio::awaitable<void>
        {
            co_stop_if(token);

            co_return;
        }
    );

    layout::DockSpace dock_space;

    panel::DrawContext ctx;

    panel::MainMenuBar main_menu_bar;
    panel::ScenePanel scene_panel;
    panel::PropertiesPanel properties_panel;
    panel::AssetsPanel assets_panel;
    panel::ViewportPanel viewport_panel;

    orchestrator.setRenderStage<0>(
    
        [&app_state]
        (async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr) -> asio::awaitable<void>
        {
            co_stop_if(token);
            co_await app_state.renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f});
        }
    );

    orchestrator.setRenderStage<1>(
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