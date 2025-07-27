#include <astre.hpp>

namespace astre::entry
{
    // simulation for frame N
    static asio::awaitable<void> _simulateFrame(const ecs::Systems & systems, render::Frame & frame)
    {
        co_await systems.transform.run();
        co_await systems.camera.run(frame);

        co_await systems.visual.run(frame);
        co_await systems.light.run(frame);

        co_return;
    }
    
    asio::awaitable<void> gameMain(const pipeline::GamePipelineState & game_state, const entry::AppPaths & paths)
    {
        co_await asset::loadVertexBuffersPrefabs(game_state.app_state.renderer);
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_shader");
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "deferred_lighting_pass");
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "shadow_depth");
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "basic_shader");
        co_await asset::loadShaderFromDir(game_state.app_state.renderer, paths.resources / "shaders" / "glsl" / "simple_NDC");

        auto deferred_fbo_res = co_await game_state.app_state.renderer.createFrameBufferObject("fbo::deferred",
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

        // binding point 2 in shader
        auto light_ssbo_res = co_await game_state.app_state.renderer.createShaderStorageBuffer("ssbo::light", 2, 0, nullptr);
        if(!light_ssbo_res)
        {
            spdlog::error("Failed to create light shader storage buffer");
            co_return;
        }
        const std::size_t & light_ssbo = *light_ssbo_res;

        auto shadow_shader_res = game_state.app_state.renderer.getShader("shadow_depth");
        if(!shadow_shader_res)
        {
            spdlog::error("Failed to get shadow pass shader");
            co_return;
        }
        const std::size_t & shadow_shader = *shadow_shader_res;

        // create shadow maps
        std::vector<std::size_t> shadow_map_fbos;
        for(unsigned int i = 0; i < ecs::system::LightSystem::MAX_SHADOW_CASTERS; ++i)
        {
            const auto new_fbo = co_await game_state.app_state.renderer.createFrameBufferObject(
                "fbo::shadow_map" + std::to_string(i), std::make_pair(1280, 728),
                {{render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth_32F}});
            
            if(!new_fbo)
            {
                spdlog::error("Failed to create shadow map fbo {}", i);
                co_return;
            }

            shadow_map_fbos.emplace_back(*new_fbo);
        }

        // fetch shadow map textures from frame buffer objects
        std::vector<std::size_t> shadow_map_textures;
        shadow_map_textures.reserve(ecs::system::LightSystem::MAX_SHADOW_CASTERS);
        for(const auto & shader_map_fbo : shadow_map_fbos)
        {
            auto tex_res = game_state.app_state.renderer.getFrameBufferObjectTextures(shader_map_fbo);
            assert(tex_res.size() == 1 && "Shadow map fbo should only have one texture" );
            shadow_map_textures.emplace_back(tex_res.at(0));
        }

        auto NDC_quad_res = game_state.app_state.renderer.getVertexBuffer("NDC_quad_prefab");
        if(!NDC_quad_res)
        {
            spdlog::error("Failed to load NDC_quad_prefab");
            co_return;
        }
        const std::size_t & NDC_quad = *NDC_quad_res;

        const auto NDC_shader_res = game_state.app_state.renderer.getShader("deferred_lighting_pass");
        if(!NDC_shader_res)
        {
            spdlog::error("Failed to get NDC shader");
            co_return;
        } 
        const std::size_t & NDC_shader = *NDC_shader_res;


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
        float alpha = 0.0f;
        std::chrono::steady_clock::time_point now;

        auto camera_entity = game_state.registry.getEntity("camera");
        if(!camera_entity)
        {
            spdlog::error("Failed to get camera entity");
            co_return;
        }
        const auto & camera = *camera_entity;
        float camera_rot = 0;

        while(game_state.app_state.window.good())
        {
            co_await main_loop_context.ensureOnStrand();
            
            co_await game_state.app_state.input_service.update();

            const auto pos = game_state.registry.getComponent<ecs::TransformComponent>(camera)->position();

            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_A))
            {
                spdlog::debug("A key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_x(pos.x() + 1.0f);
            }

            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_D))
            {
                spdlog::debug("D key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_x(pos.x() - 1.0f);
            }

            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_W))
            {
                spdlog::debug("W key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_z(pos.z() + 1.0f);
            }

            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_S))
            {
                spdlog::debug("S key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_z(pos.z() - 1.0f);
            }
            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_Q))
            {
                spdlog::debug("Q key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_y(pos.y() - 1.0f);
            }
            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_E))
            {
                spdlog::debug("E key pressed");
                game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_position()->set_y(pos.y() + 1.0f);
            }
            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_C))
            {
                spdlog::debug("C key pressed");
                camera_rot--;
            }
            if(game_state.app_state.input_service.isKeyPressed(input::InputCode::KEY_Z))
            {
                spdlog::debug("Z key pressed");
                camera_rot++;
            }

            game_state.registry.getComponent<ecs::TransformComponent>(camera)->mutable_rotation()->
                CopyFrom(math::serialize(glm::angleAxis(glm::radians(camera_rot), glm::vec3{0, 1, 0})));


            now = std::chrono::steady_clock::now();
            accumulator += std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            // Run simulation in fixed steps
            while (accumulator >= fixed_dt)
            {
                // Spawn simulation task for frame N
                co_await _simulateFrame(game_state.systems, game_state.frames[current_sim_index]);

                // Rotate ring buffer
                current_sim_index = (current_sim_index + 1) % 3;
                accumulator -= fixed_dt;
            }

            const render::Frame & frame = game_state.frames[current_sim_index];

            alpha = accumulator / fixed_dt;

            //  render interpolated frames N-2 and N-1
            // render to Gbuffer
            // co_await render::renderInterpolated(
            //     game_state.frames[(current_sim_index + 1) % 3],  // N−2
            //     game_state.frames[(current_sim_index + 2) % 3],  // N−1
            //     alpha,
            //     game_state.app_state.renderer,
            //     render::RenderOptions{.mode = render::RenderMode::Solid},
            //     deferred_fbo
            // );
        co_await game_state.app_state.renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, deferred_fbo);
        for(const auto & [_, proxy] : frame.render_proxies)
        {
            co_await game_state.app_state.renderer.render( 
                proxy.vertex_buffer,
                proxy.shader,
                proxy.inputs,
                render::RenderOptions{.mode = render::RenderMode::Solid},
                deferred_fbo
            );
        }

            // for every shadow caster we need to render whole scene 
            // so all entities with visual component
            // using simplified shadow shader
            const ecs::TransformComponent * transform_component = nullptr;
            const ecs::VisualComponent * visual_component = nullptr;
            std::optional<std::size_t> vb_id;
            for(std::size_t shadow_caster_id = 0; shadow_caster_id < frame.shadow_casters_count; ++shadow_caster_id)
            {
                co_await game_state.app_state.renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, shadow_map_fbos.at(shadow_caster_id));

                for(const auto & [entity, mask] : game_state.registry.getAllEntities())
                {
                    if(!mask.test(ecs::system::VisualSystem::MASK_BIT) ||
                        !mask.test(ecs::system::TransformSystem::MASK_BIT)) continue;

                    visual_component = game_state.registry.getComponent<ecs::VisualComponent>(entity);
                    transform_component = game_state.registry.getComponent<ecs::TransformComponent>(entity);
                    assert(visual_component != nullptr && transform_component != nullptr);

                    if(visual_component->visible() == false) continue;
                    if(transform_component->has_transform_matrix() == false) continue;

                    vb_id = game_state.app_state.renderer.getVertexBuffer(visual_component->vertex_buffer_name());
                    if(!vb_id) continue;

                    // render depth information to shadow map fbo
                    co_await game_state.app_state.renderer.render(*vb_id, shadow_shader, 
                        render::ShaderInputs{
                            .in_mat4 = {
                                {"uModel", math::deserialize(transform_component->transform_matrix())},
                                {"uLightSpaceMatrix", frame.light_space_matrices[shadow_caster_id]}
                            }
                        },
                        render::RenderOptions{
                            .mode = render::RenderMode::Solid,
                            .polygon_offset = render::PolygonOffset{.factor = 1.5f, .units = 4.0f}
                        },
                        shadow_map_fbos.at(shadow_caster_id)
                    );
                }
            }
            
            co_await game_state.app_state.renderer.updateShaderStorageBuffer(
                light_ssbo, sizeof(render::GPULight) * frame.gpu_lights.size(), frame.gpu_lights.data());

            // render GBuffer to screen
            co_await game_state.app_state.renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f});
            co_await game_state.app_state.renderer.render(NDC_quad, NDC_shader,
                render::ShaderInputs{
                    .in_uint = {
                        {"lightCount", (std::uint32_t)frame.gpu_lights.size()},
                        {"shadowCastersCount", frame.shadow_casters_count}
                    },
                    .in_mat4_array = {
                        {"lightSpaceMatrices", frame.light_space_matrices}
                    },
                    .in_samplers = {
                        {"gPosition",   gbuffer_textures.at(0)},
                        {"gNormal",     gbuffer_textures.at(1)},
                        {"gAlbedoSpec", gbuffer_textures.at(2)}
                    },
                    .in_samplers_array = {
                        {"shadowMaps", shadow_map_textures}
                    },
                    .storage_buffers = {
                        light_ssbo
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
        spdlog::debug("[pipeline] GamePipeline running");
        co_await pipeline::GamePipeline{app_state}.run<void>(gameMain, paths);
        spdlog::debug("[pipeline] GamePipeline ended");
    }

    asio::awaitable<int> main(const entry::AppPaths & paths, process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");
        
        spdlog::debug("[pipeline] WindowApp running");
        co_await pipeline::WindowApp{process, "astre game", 1280, 720}.run<void>(appMain, paths);
        spdlog::debug("[pipeline] WindowApp ended");

        co_return 0;
    }
}