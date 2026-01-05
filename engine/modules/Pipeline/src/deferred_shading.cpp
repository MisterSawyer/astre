#include "pipeline/deferred_shading.hpp"

#include "ecs/ecs.hpp"

namespace astre::pipeline
{
    asio::awaitable<std::expected<DeferredShadingResources, bool>> buildDeferredShadingResources(render::IRenderer & renderer, std::pair<unsigned,unsigned> size)
    {
        DeferredShadingResources resources;

        resources.gbuffer_render_options =
            render::RenderOptions{
                .mode = render::RenderMode::Solid
            };

        resources.shadow_map_render_options =
            render::RenderOptions{
                .mode = render::RenderMode::Solid,
                .polygon_offset = render::PolygonOffset{.factor = 1.5f, .units = 4.0f}
            };

        // Create deffered FBO ( GBuffer )
        auto deferred_fbo_res = co_await renderer.createFrameBufferObject(
            "fbo::deferred", size,
            {
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGBA_16F},
                {render::FBOAttachment::Type::RenderBuffer, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth}
            }
        );
        if (!deferred_fbo_res) {
            spdlog::error("Failed to create deferred FBO");
            co_return std::unexpected(false);
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
            co_return std::unexpected(false);
        }

        resources.light_ssbo = *light_ssbo_res;

        // Create shadow maps
        for (unsigned int i = 0; i < ecs::system::LightSystem::MAX_SHADOW_CASTERS; ++i) 
        {
            auto fbo = co_await renderer.createFrameBufferObject(
                "fbo::shadow_map" + std::to_string(i), size,
                {{render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Depth, render::TextureFormat::Depth_32F}}
            );

            if (!fbo) {
                spdlog::error("Failed to create shadow map FBO {}", i);
                co_return std::unexpected(false);
            }

            resources.shadow_map_fbos.emplace_back(*fbo);
        }
        // obtain shadow maps depth textures
        for (auto& shadow_fbo : resources.shadow_map_fbos) 
        {
            auto tex = renderer.getFrameBufferObjectTextures(shadow_fbo);
            assert(tex.size() == 1 && "Shadow map FBO should have 1 texture");
            resources.shadow_map_textures.emplace_back(tex[0]);
        }

        // obtain shadow pass shader
        auto shadow_shader_res = renderer.getShader("shadow_depth");
        if(!shadow_shader_res)
        {
            spdlog::error("Failed to get shadow pass shader");
            co_return std::unexpected(false);
        }
        resources.shadow_map_shader = *shadow_shader_res;

        // obtain screen quad vertex buffer
        auto screen_quad_vb_res = renderer.getVertexBuffer("NDC_quad_prefab");
        if(!screen_quad_vb_res)
        {
            spdlog::error("Failed to get screen quad vertex buffer");
            co_return std::unexpected(false);
        }
        resources.screen_quad_vb = *screen_quad_vb_res;

        // obtain screen quad shader
        auto screen_quad_shader_res = renderer.getShader("deferred_lighting_pass");
        if(!screen_quad_shader_res)
        {
            spdlog::error("Failed to get screen quad shader");
            co_return std::unexpected(false);
        }
        resources.screen_quad_shader = *screen_quad_shader_res;

        co_return resources;
    }

    static asio::awaitable<render::FrameStats> _renderFrameToGBuffer(
            render::IRenderer & renderer,
            const render::Frame & frame,
            const DeferredShadingResources & resources)
    {
        // clear GBuffer
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.deferred_fbo);
        
        render::FrameStats stats;
        render::ShaderInputs inputs;

        for(const auto & [_, proxy] : frame.render_proxies)
        {
            if(proxy.visible == false)continue;

            //only draw those that are in opaque phase
            if (!render::hasFlags(proxy.phases & render::RenderPhase::Opaque)) continue;

            inputs = proxy.inputs;
            inputs.in_mat4["uView"] = frame.view_matrix;
            inputs.in_mat4["uProjection"] = frame.proj_matrix;
            stats += co_await renderer.render(
                proxy.vertex_buffer,
                proxy.shader,
                inputs,
                resources.gbuffer_render_options,
                resources.deferred_fbo
            );
        }

        co_return stats;
    }

    static asio::awaitable<render::FrameStats> _renderFrameToShadowMaps(
            render::IRenderer & renderer,
            const render::Frame & frame,
            const DeferredShadingResources & resources)
    {
        render::FrameStats stats;
        // for every shadow caster we need to render whole scene 
        // using simplified shadow shader
        for(std::size_t shadow_caster_id = 0; shadow_caster_id < resources.shadow_map_fbos.size(); ++shadow_caster_id)
        {
            // clear shadow map
            co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.shadow_map_fbos.at(shadow_caster_id));

            // render scene to shadow map
            for(const auto & [_, proxy] : frame.render_proxies)
            {
                if(proxy.visible == false)continue;

                //only draw true casters
                if (!render::hasFlags(proxy.phases & render::RenderPhase::ShadowCaster)) continue;

                if(shadow_caster_id >= frame.light_space_matrices.size())
                {
                    continue;
                }
                // render depth information to shadow map fbo
                stats += co_await renderer.render(proxy.vertex_buffer, resources.shadow_map_shader,
                    render::ShaderInputs{
                        .in_mat4 = {
                            {"uModel", proxy.inputs.in_mat4.at("uModel")},
                            {"uLightSpaceMatrix", frame.light_space_matrices.at(shadow_caster_id)}
                        }
                    },
                    resources.shadow_map_render_options,
                    resources.shadow_map_fbos.at(shadow_caster_id)
                );
            }
        }

        co_return stats;
    }
    
    static asio::awaitable<render::FrameStats> _renderGBuffer(    
        render::IRenderer & renderer,
        const render::Frame & frame,
        const DeferredShadingResources & resources,
        std::optional<std::size_t> fbo)
    {
        render::FrameStats stats;

        // clear screen
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, fbo);

        // render GBuffer to fbo or if no fbo is provided render to screen
        stats += co_await renderer.render(resources.screen_quad_vb, resources.screen_quad_shader,
            render::ShaderInputs{
            .in_uint = {
                {"lightCount", (std::uint32_t)frame.gpu_lights.size()},
                {"shadowCastersCount", frame.shadow_casters_count}
            },
            .in_mat4_array = {
                {"lightSpaceMatrices", frame.light_space_matrices}
            },
            .in_samplers = {
                {"gPosition",   resources.deferred_textures.at(0)},
                {"gNormal",     resources.deferred_textures.at(1)},
                {"gAlbedoSpec", resources.deferred_textures.at(2)}
            },
            .in_samplers_array = {
                {"shadowMaps", resources.shadow_map_textures}
            },
            .storage_buffers = {
                resources.light_ssbo
            }
            },
            render::RenderOptions{
                .mode = render::RenderMode::Solid
            },
            fbo
        );

        co_return stats;
    }

    asio::awaitable<render::FrameStats> deferredShadingStage(
            render::IRenderer & renderer,
            const DeferredShadingResources & render_resources,
            const render::Frame & frame,
            std::optional<std::size_t> fbo)
    {
        // update light SSBO
        std::vector<render::GPULight> lights_buffer;
        lights_buffer.reserve(frame.gpu_lights.size());
        for (auto& [e, light] : frame.gpu_lights) {
            lights_buffer.emplace_back(std::move(light));
        }
         co_await renderer.updateShaderStorageBuffer(
              render_resources.light_ssbo, sizeof(render::GPULight) * lights_buffer.size(), lights_buffer.data());
        
        render::FrameStats stats;
        stats += co_await _renderFrameToGBuffer(renderer, frame, render_resources);
        stats += co_await _renderFrameToShadowMaps(renderer, frame, render_resources);
        stats += co_await _renderGBuffer(renderer, frame, render_resources, fbo);

        co_return stats;
    }
}