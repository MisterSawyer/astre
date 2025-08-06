#include "pipeline/pipeline.hpp"

namespace astre::pipeline
{
    asio::awaitable<void> renderFrameToGBuffer(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, const render::RenderOptions & options, render::FrameStats * stats)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(async::isCancelled(cs)) co_return;
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.deferred_fbo);
        
        for(const auto & [_, proxy] : frame.render_proxies)
        {
            assert(cs.slot().is_connected());

            if(async::isCancelled(cs)) co_return;
            try{
                co_await renderer.render(
                    proxy.vertex_buffer,
                    proxy.shader,
                    proxy.inputs,
                    options,
                    resources.deferred_fbo,
                    stats
                );
            }catch (const asio::system_error& e)
            {
                if (e.code() == asio::error::operation_aborted) {
                    spdlog::debug("[pipeline] cancelled during render");
                    co_return;
                }
                throw;
            }
        }  
    }

    asio::awaitable<void> renderFrameToShadowMaps(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, const render::RenderOptions & options, render::FrameStats * stats)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        // for every shadow caster we need to render whole scene 
        // using simplified shadow shader
        for(std::size_t shadow_caster_id = 0; shadow_caster_id < resources.shadow_map_fbos.size(); ++shadow_caster_id)
        {
            // clear shadow map
            if(async::isCancelled(cs)) co_return;
            co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f}, resources.shadow_map_fbos.at(shadow_caster_id));

            // render scene to shadow map
            for(const auto & [_, proxy] : frame.render_proxies)
            {
                if(shadow_caster_id >= frame.light_space_matrices.size())
                {
                    continue;
                }

                if(async::isCancelled(cs)) co_return;
                try{
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
                catch (const asio::system_error& e)
                {
                    if (e.code() == asio::error::operation_aborted) {
                        spdlog::debug("[pipeline] cancelled during render");
                        co_return;
                    }
                    throw;
                }
            }
        }
    }
    
    asio::awaitable<void> renderGBufferToScreen(render::IRenderer & renderer, const render::Frame & frame, RenderResources & resources, render::FrameStats * stats)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        // clear screen
        if(async::isCancelled(cs)) co_return;
        co_await renderer.clearScreen({0.0f, 0.0f, 0.0f, 1.0f});
        
        if(async::isCancelled(cs)) co_return;
        try{
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
        catch (const asio::system_error& e)
        {
            if (e.code() == asio::error::operation_aborted) {
                spdlog::debug("[pipeline] cancelled during render");
                co_return;
            }
            throw;
        }
    }
}