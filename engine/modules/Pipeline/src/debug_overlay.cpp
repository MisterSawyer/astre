#include "pipeline/debug_overlay.hpp"

namespace astre::pipeline
{

    asio::awaitable<std::optional<DebugOverlayResources>> buildDebugOverlayResources(render::IRenderer & renderer)
    {
        DebugOverlayResources resources;

        auto debug_shader_res = renderer.getShader("debug_overlay");
        if (!debug_shader_res) {
            spdlog::warn("No 'debug_overlay' shader; debug phase will skip.");
            co_return std::nullopt;
        } else {
            resources.debug_overlay_shader = *debug_shader_res;
        }

        co_return resources;
    }

    asio::awaitable<render::FrameStats> renderDebugOverlay(
        render::IRenderer & renderer,
        const DebugOverlayResources & resources,
        const render::Frame & frame,
        std::optional<std::size_t> fbo)
    {
        render::FrameStats stats;

        // Early out if no shader or no debug proxies
        if (!resources.debug_overlay_shader) co_return stats;
       
        render::ShaderInputs inputs;

        for (const auto& [_, proxy] : frame.render_proxies)
        {
            if (!proxy.visible) continue;

            // only draw those that are in debug phase
            if (!render::hasFlags(proxy.phases & render::RenderPhase::Debug)) continue;

            inputs = proxy.inputs;
            inputs.in_mat4["uView"] = frame.view_matrix;
            inputs.in_mat4["uProjection"] = frame.proj_matrix;

            stats += co_await renderer.render(
                proxy.vertex_buffer,
                resources.debug_overlay_shader,
                inputs,
                proxy.options,
                fbo
            );
        }

        co_return stats;
    }
}