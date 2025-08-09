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

        pipeline::RenderResources render_resources;

        world::WorldStreamer world;
    };

    asio::awaitable<pipeline::RenderResources> buildRenderResources(render::IRenderer & renderer, pipeline::FramesBuffer<GameFrame> & buffer)
    {
        pipeline::RenderResources resources;

        // Create deffered FBO ( GBuffer )
        auto deferred_fbo_res = co_await renderer.createFrameBufferObject(
            "fbo::deferred", {1280, 728},
            {
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGBA_16F},
                {render::FBOAttachment::Type::RenderBuffer, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth}
            }
        );
        if (!deferred_fbo_res) {
            spdlog::error("Failed to create deferred FBO");
            // TODO throw?
            co_return pipeline::RenderResources{};  // return empty
        }
        resources.deferred_fbo = *deferred_fbo_res;
        // obtain deferred fbo textures
        resources.deferred_textures = renderer.getFrameBufferObjectTextures(resources.deferred_fbo);
        assert(resources.deferred_textures.size() == 3);

        // Create SSBO for lights
        std::optional<std::size_t> light_ssbo_res;
        light_ssbo_res = co_await renderer.createShaderStorageBuffer("ssbo::light", 2, 0, nullptr);
        if (!light_ssbo_res) {
            spdlog::error("Failed to create light SSBO");
            // TODO throw?
            co_return pipeline::RenderResources{};
        }
        
        for(std::size_t i = 0; i < buffer.size(); ++i)
        {
            buffer.at(i).render_frame.light_ssbo = *light_ssbo_res;
        }

        // Create shadow maps
        for (unsigned int i = 0; i < ecs::system::LightSystem::MAX_SHADOW_CASTERS; ++i) {
            auto fbo = co_await renderer.createFrameBufferObject(
                "fbo::shadow_map" + std::to_string(i), {1280, 728},
                {{render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth_32F}}
            );

            if (!fbo) {
                spdlog::error("Failed to create shadow map FBO {}", i);
                // TODO throw?
                co_return pipeline::RenderResources{};
            }

            resources.shadow_map_fbos.emplace_back(*fbo);
        }
        // obtain shadow maps depth textures
        for (auto& shadow_fbo : resources.shadow_map_fbos) {
            auto tex = renderer.getFrameBufferObjectTextures(shadow_fbo);
            assert(tex.size() == 1 && "Shadow map FBO should have 1 texture");
            resources.shadow_map_textures.emplace_back(tex[0]);
        }

        // obtain shadow pass shader
        auto shadow_shader_res = renderer.getShader("shadow_depth");
        if(!shadow_shader_res)
        {
            spdlog::error("Failed to get shadow pass shader");
            // TODO throw?
            co_return pipeline::RenderResources{};
        }
        resources.shadow_map_shader = *shadow_shader_res;

        // obtain screen quad vertex buffer
        auto screen_quad_vb_res = renderer.getVertexBuffer("NDC_quad_prefab");
        if(!screen_quad_vb_res)
        {
            spdlog::error("Failed to get screen quad vertex buffer");
            co_return pipeline::RenderResources{};
        }
        resources.screen_quad_vb = *screen_quad_vb_res;

        // obtain screen quad shader
        auto screen_quad_shader_res = renderer.getShader("deferred_lighting_pass");
        if(!screen_quad_shader_res)
        {
            spdlog::error("Failed to get screen quad shader");
            co_return pipeline::RenderResources{};
        }
        resources.screen_quad_shader = *screen_quad_shader_res;

        co_return resources;
    }

    float logic_fps = 0.0f;
    float logic_frame_time = 0.0f;
    constexpr float logic_frame_smoothing = 0.9f;
    std::chrono::steady_clock::time_point logic_last_time;
    std::chrono::steady_clock::time_point logic_start;

    asio::awaitable<void> logicStep0(async::LifecycleToken& token, float dt, GameFrame& frame, GameState& state)
    {
        co_stop_if(token);

        auto & ex = state.app_state.process.getExecutionContext();
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

        logic_last_time = logic_start;
        logic_start = std::chrono::steady_clock::now();

        // --- Input + world in parallel (explicitly handle exceptions) ---
        {
            auto g = asio::experimental::make_parallel_group(
                asio::co_spawn(ex, state.app_state.input.update(token),                    asio::deferred),
                asio::co_spawn(ex, state.world.updateLoadPosition({0.0f, 0.0f, 0.0f}),    asio::deferred)
            );

            auto [ord, e1, e2] =
                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

            if (e1) std::rethrow_exception(e1);
            if (e2) std::rethrow_exception(e2);
        }

        //--- ECS stage ---
        {
            auto g = asio::experimental::make_parallel_group(
                asio::co_spawn(ex, state.systems.transform.run(dt), asio::deferred),
                asio::co_spawn(ex, state.systems.input.run(dt), asio::deferred)
            );

            auto [ord, e1, e2] =
                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

            if (e1) std::rethrow_exception(e1);
            if (e2) std::rethrow_exception(e2);
        }

        // Serial steps
        state.systems.script.run(dt);
        state.systems.camera.run(dt, frame.render_frame);

        // --- Visual + Light in parallel (same executor, explicit errors) ---
        {
            auto g = asio::experimental::make_parallel_group(
                asio::co_spawn(ex, state.systems.visual.run(dt, frame.render_frame), asio::deferred),
                asio::co_spawn(ex, state.systems.light.run(dt,  frame.render_frame), asio::deferred)
            );

            auto [ord, e1, e2] =
                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

            if (e1) std::rethrow_exception(e1);
            if (e2) std::rethrow_exception(e2);
        }

        const float frame_time_ms =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - logic_start).count() * 1000.0f;
        logic_frame_time = (logic_frame_time * logic_frame_smoothing) + (frame_time_ms * (1.0f - logic_frame_smoothing));

        const float current_fps =
            1.0f / std::max(std::chrono::duration<float>(logic_start - logic_last_time).count(), 1e-6f);
        logic_fps = (logic_fps * logic_frame_smoothing) + (current_fps * (1.0f - logic_frame_smoothing));

        co_return;
    }


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
    asio::awaitable<void> renderStage(async::LifecycleToken & token, float alpha, const render::Frame & prev, const render::Frame & curr, GameState & state)
    {
        co_stop_if(token);

        // Render interpolated frame using prev <-> curr, using alpha
        auto interpolated_frame = render::interpolateFrame(prev, curr, alpha);



        co_await pipeline::renderFrameToGBuffer(state.app_state.renderer, interpolated_frame, state.render_resources, gbuffer_render_options, &render_stats);
        co_await pipeline::renderFrameToShadowMaps(state.app_state.renderer, interpolated_frame, state.render_resources, shadow_map_render_options, &render_stats);
        
        // update light SSBO
        std::vector<render::GPULight> lights_buffer;
        lights_buffer.reserve(interpolated_frame.gpu_lights.size());
        for (auto& [e, light] : interpolated_frame.gpu_lights) {
            lights_buffer.push_back(std::move(light));
        }
         co_await state.app_state.renderer.updateShaderStorageBuffer(
              interpolated_frame.light_ssbo, sizeof(render::GPULight) * lights_buffer.size(), lights_buffer.data());

        co_await pipeline::renderGBufferToScreen(state.app_state.renderer, interpolated_frame, state.render_resources, &render_stats); 

        // GUI
        co_await state.app_state.gui.newFrame();
        co_await state.app_state.gui.draw(gui::drawDebugOverlay, logic_fps, logic_frame_time, std::cref(render_stats));
        co_await state.app_state.gui.draw(gui::drawRenderControlWindow, std::ref(gbuffer_render_options), std::ref(shadow_map_render_options));
        co_await state.app_state.gui.render();

        // clear stats
        render_stats.draw_calls = 0;
        render_stats.vertices = 0;
        render_stats.triangles = 0;

        co_return;
    }

    asio::awaitable<void> syncStage(async::LifecycleToken & token, GameState & state)
    {
        co_stop_if(token);
        co_await state.app_state.renderer.present(); // Swap buffers
    }

    asio::awaitable<void> runMainLoop(async::LifecycleToken & token, pipeline::AppState app_state, const entry::AppPaths & paths)
    {
        co_await app_state.renderer.enableVSync();

        co_await asset::loadVertexBuffersPrefabs(app_state.renderer);
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");

        script::ScriptRuntime script_runtime;
        co_await asset::loadScript(script_runtime, paths.resources / "worlds" / "scripts" / "player_script.lua");

        ecs::Registry registry(app_state.process.getExecutionContext());

        pipeline::FramesBuffer<GameFrame> buffer;
        pipeline::RenderResources render_resources = co_await buildRenderResources(app_state.renderer, buffer);
        pipeline::PipelineOrchestrator<GameFrame, GameState> orchestrator(app_state.process, buffer,
            GameState
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
                .render_resources = std::move(render_resources),
                .world = world::WorldStreamer(  
                    app_state.process.getExecutionContext(),
                    paths.resources / "worlds/levels/level_0.json",
                    registry,
                    32.0f, 32)
            }
        );
        
        orchestrator.setLogicSubstage<0>(logicStep0);

        orchestrator.setRenderStage(renderStage);

        orchestrator.setPostSync(syncStage);

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