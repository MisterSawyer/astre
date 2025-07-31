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

        // Create SSBO for lights in all frames
        std::optional<std::size_t> light_ssbo_res;
        for(std::size_t i = 0; i < buffer.size(); ++i)
        {
            light_ssbo_res = co_await renderer.createShaderStorageBuffer("ssbo::light" + std::to_string(i), 2, 0, nullptr);
            if (!light_ssbo_res) {
                spdlog::error("Failed to create light SSBO [{}]", i);
                // TODO throw?
                co_return pipeline::RenderResources{};
            }
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

    asio::awaitable<void> runMainLoop(pipeline::AppState app_state, const entry::AppPaths & paths)
    {
        co_await app_state.renderer.enableVSync();

        co_await asset::loadVertexBuffersPrefabs(app_state.renderer);
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
        co_await asset::loadShaderFromDir(app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");

        ecs::Registry registry(app_state.process.getExecutionContext());

        pipeline::FramesBuffer<GameFrame> buffer;
        pipeline::RenderResources render_resources = co_await buildRenderResources(app_state.renderer, buffer);
        pipeline::PipelineOrchestrator<GameFrame, GameState> orchestrator(app_state.process, buffer,
            GameState
            {
                .app_state = app_state,
                .registry = registry,
                .systems = ecs::Systems{
                    .transform = ecs::system::TransformSystem(registry, app_state.process.getExecutionContext()),
                    .camera = ecs::system::CameraSystem(registry, app_state.process.getExecutionContext(), "camera"),
                    .visual = ecs::system::VisualSystem(app_state.renderer, registry, app_state.process.getExecutionContext()),
                    .light = ecs::system::LightSystem(registry, app_state.process.getExecutionContext())
                },
                .render_resources = std::move(render_resources),
                .world = world::WorldStreamer(  
                    app_state.process.getExecutionContext(),
                    paths.resources / "worlds/levels/level_0.json",
                    registry,
                    32.0f, 32)
            }
        );
        
        orchestrator.setLogicSubstage<0>([&](GameFrame & frame, GameState & state) -> asio::awaitable<void>
        {
            asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Logic stage cancelled");
                co_return;
            }
            co_await (state.app_state.input.update() && state.world.updateLoadPosition({0.0f, 0.0f, 0.0f}));
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Logic stage cancelled");
                co_return;
            }
            // ECS stage
            co_await state.systems.transform.run();
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Logic stage cancelled");
                co_return;
            }
            co_await state.systems.camera.run(frame.render_frame);
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Logic stage cancelled");
                co_return;
            }
            // 
            co_await (state.systems.visual.run(frame.render_frame) && state.systems.light.run(frame.render_frame));
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Logic stage cancelled");
                co_return;
            }
            // update light SSBO
            co_await state.app_state.renderer.updateShaderStorageBuffer(
                  frame.render_frame.light_ssbo, sizeof(render::GPULight) * frame.render_frame.gpu_lights.size(), frame.render_frame.gpu_lights.data());
        });

        orchestrator.setRenderStage([&](const GameFrame& prev, const GameFrame& curr, GameState & state) -> asio::awaitable<void> {
            asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Render stage cancelled");
                co_return;
            }
            // Render interpolated frame using prev + curr
            co_await pipeline::renderFrameToGBuffer(state.app_state.renderer, curr.render_frame, state.render_resources);
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Render stage cancelled");
                co_return;
            }
            co_await pipeline::renderFrameToShadowMaps(state.app_state.renderer, curr.render_frame, state.render_resources);
            
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("Render stage cancelled");
                co_return;
            }
            co_await pipeline::renderGBufferToScreen(state.app_state.renderer, curr.render_frame, state.render_resources); 

            co_return;
        });

        orchestrator.setPostSync([&](GameState & state) -> asio::awaitable<void> {
            co_await app_state.renderer.present(); // Swap buffers
        });

        co_await orchestrator.runLoop(app_state.lifecycle);
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