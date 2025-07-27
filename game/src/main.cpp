#include <astre.hpp>

namespace astre::entry
{
    // simulation for frame N
    static asio::awaitable<void> _simulateFrame(const ecs::Systems & systems, render::Frame & frame)
    {
        co_await systems.transform.run();
        co_await systems.camera.run(frame);

        co_await systems.visual.run(frame);

        co_return;
    }
    
    asio::awaitable<void> gameMain(const pipeline::GamePipelineState & game_state, const entry::AppPaths & paths)
    {
        co_await asset::loadVertexBuffersPrefabs(game_state.app_state.renderer);
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");

        auto deferred_fbo_res = co_await game_state.app_state.renderer.createFrameBufferObject("deferred_fbo",
            std::make_pair(1280, 728), 
            {
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},    // gPosition
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},    // gNormal
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGBA_16F},   // gAlbedo
                {render::FBOAttachment::Type::RenderBuffer, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth}  // gDepth
            }
        );
        if(!deferred_fbo_res)
        {
            spdlog::error("Failed to create deferred FBO");
            co_return;
        }
        const std::size_t & deferred_fbo = *deferred_fbo_res;
        const auto & gbuffer_textures = game_state.app_state.renderer.getFrameBufferObjectTextures(deferred_fbo);
        assert(gbuffer_textures.size() == 3);

        auto NDC_quad_res = game_state.app_state.renderer.getVertexBuffer("NDC_quad_prefab");
        if(!NDC_quad_res)
        {
            spdlog::error("Failed to load NDC_quad_prefab");
            co_return;
        }
        const std::size_t & NDC_quad = *NDC_quad_res;

        const auto deferred_lighting_pass_res = game_state.app_state.renderer.getShader("deferred_lighting_pass");
        if(!deferred_lighting_pass_res)
        {
            spdlog::error("Failed to get deferred lighting pass shader");
            co_return;
        } 
        const std::size_t & deferred_lighting_pass = *deferred_lighting_pass_res;


        co_await game_state.app_state.renderer.enableVSync();

        game_state.systems.camera.setActiveCameraEntityName("camera");

        async::AsyncContext main_loop_context(game_state.app_state.process.getExecutionContext());

        world::WorldStreamer world_streamer{
            main_loop_context,
            paths.resources / "worlds/levels/level_0.json",
            game_state.registry, game_state.loader, game_state.serializer, 
            16.0f, 8};

        co_await world_streamer.updateLoadPosition({0.0f, 0.0f, 0.0f});

        constexpr float fixed_dt = 1.0f / 20.0f;
        float accumulator = 0.0f;
        std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
        
        int current_sim_index = 0;  // this is N
        int render_from_index = (current_sim_index + 1) % 3; // N-2
        int render_to_index   = (current_sim_index + 2) % 3; // N-1

        std::chrono::steady_clock::time_point now;

        while(game_state.app_state.window.good())
        {
            co_await main_loop_context.ensureOnStrand();

            now = std::chrono::steady_clock::now();
            accumulator += std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            // Run simulation in fixed steps
            while (accumulator >= fixed_dt)
            {
                // Spawn simulation task for frame N
                main_loop_context.co_spawn(_simulateFrame(game_state.systems, game_state.frames[current_sim_index]));

                // Rotate ring buffer
                current_sim_index = (current_sim_index + 1) % 3;
                accumulator -= fixed_dt;
            }

            co_await game_state.app_state.renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f});
            
            // Spawn render task for frames N-2 and N-1
            // render to Gbuffer
            main_loop_context.co_spawn(
                render::renderInterpolated(
                    game_state.frames[(current_sim_index + 1) % 3],  // N−2
                    game_state.frames[(current_sim_index + 2) % 3],  // N−1
                    accumulator / fixed_dt,
                    game_state.app_state.renderer,
                    render::RenderOptions{.mode = render::RenderMode::Solid},
                    deferred_fbo
                    ));

            // render GBuffer to screen
            co_await game_state.app_state.renderer.render(NDC_quad, deferred_lighting_pass,
                render::ShaderInputs{
                    .in_int = {
                        //{"lightCount", (int)light_system.getLightsCount()},
                        //{"shadowCastersCount", (int)light_system.getShadowCastersCount()}
                    },
                    .in_mat4_array = {
                        //{"lightSpaceMatrices", light_system.getShadowMapLightSpaceMatrices()}
                    },
                    .in_samplers = {
                        {"gPosition",   gbuffer_textures.at(0)},
                        {"gNormal",     gbuffer_textures.at(1)},
                        {"gAlbedoSpec", gbuffer_textures.at(2)}
                    },
                    .in_samplers_array = {
                        //{"shadowMaps", light_system.getShadowMapTextures()}
                    },
                    .storage_buffers = {
                        //light_system.getLightShaderBufferID()
                    }
                },
                render::RenderOptions{
                    .mode = render::RenderMode::Solid
                }
            );

            co_await game_state.app_state.renderer.present();
        };
    }

    asio::awaitable<void> appMain(const pipeline::WindowAppState & app_state, const entry::AppPaths & paths)
    {
        co_await pipeline::GamePipeline{app_state}.run<void>(gameMain, paths);
    }

    asio::awaitable<int> main(const entry::AppPaths & paths, process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");

        co_await pipeline::WindowApp{process, "astre game", 1280, 720}.run<void>(appMain, paths);

        co_return 0;
    }
}