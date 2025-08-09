#include "pipeline/pipeline.hpp"

namespace astre::pipeline
{
    asio::awaitable<void> renderFrameToGBuffer(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, const render::RenderOptions & options, render::FrameStats * stats)
    {
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.deferred_fbo);
        
        for(const auto & [_, proxy] : frame.render_proxies)
        {
            co_await renderer.render(
                proxy.vertex_buffer,
                proxy.shader,
                proxy.inputs,
                options,
                resources.deferred_fbo,
                stats
            );
        }  
    }

    asio::awaitable<void> renderFrameToShadowMaps(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, const render::RenderOptions & options, render::FrameStats * stats)
    {
        // for every shadow caster we need to render whole scene 
        // using simplified shadow shader
        for(std::size_t shadow_caster_id = 0; shadow_caster_id < resources.shadow_map_fbos.size(); ++shadow_caster_id)
        {
            // clear shadow map
            co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.shadow_map_fbos.at(shadow_caster_id));

            // render scene to shadow map
            for(const auto & [_, proxy] : frame.render_proxies)
            {
                if(shadow_caster_id >= frame.light_space_matrices.size())
                {
                    continue;
                }
                // render depth information to shadow map fbo
                co_await renderer.render(proxy.vertex_buffer, resources.shadow_map_shader,
                    render::ShaderInputs{
                        .in_mat4 = {
                            {"uModel", proxy.inputs.in_mat4.at("uModel")},
                            {"uLightSpaceMatrix", frame.light_space_matrices.at(shadow_caster_id)}
                        }
                    },
                    options,
                    resources.shadow_map_fbos.at(shadow_caster_id),
                    stats
                );
            }
        }
    }
    
    asio::awaitable<void> renderGBufferToScreen(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, render::FrameStats * stats)
    {
        // clear screen
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f});

        // render GBuffer to screen
        co_await renderer.render(resources.screen_quad_vb, resources.screen_quad_shader,
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
                frame.light_ssbo
            }
            },
            render::RenderOptions{
            .mode = render::RenderMode::Solid
            },
            std::nullopt,
            stats
        );
    }
}