#include <spdlog/spdlog.h>

#include "pipeline/picking.hpp"


namespace astre::pipeline
{
    asio::awaitable<std::optional<PickingResources>>
        buildPickingResources(render::IRenderer& renderer, std::pair<unsigned,unsigned> size)
    {
        PickingResources out;
        out.size = size;

        // shader
        auto picking_shader_res = renderer.getShader("picking_id64");
        if (!picking_shader_res) co_return std::nullopt;
        out.shader = *picking_shader_res;

        // FBO: color0 = R32UI, depth = Depth
        auto fbo_res = co_await renderer.createFrameBufferObject(
            "fbo::picking",
            size,
            {
                render::FBOAttachment{render::FBOAttachment::Type::Texture,      render::FBOAttachment::Point::Color, render::TextureFormat::RG_32},
                render::FBOAttachment{render::FBOAttachment::Type::RenderBuffer, render::FBOAttachment::Point::Depth,  render::TextureFormat::Depth}
            }
        );
        if (!fbo_res) co_return std::nullopt;

        out.fbo = *fbo_res;

        co_return out;
    }

    asio::awaitable<render::FrameStats>
        renderPickingIds(render::IRenderer& renderer, const render::Frame& frame,
                               const PickingResources& res)
    {
        render::FrameStats stats;
        if (res.fbo == 0 || res.shader == 0) co_return stats;

        // clear to 0 = "no hit"
        co_await renderer.clearScreen({0,0,0,0}, res.fbo);

        render::RenderOptions opts{
            .mode = render::RenderMode::Solid,
            .polygon_offset = std::nullopt,
            .write_depth = true
        };

        render::ShaderInputs in;
        in.in_mat4["uView"] = frame.view_matrix;
        in.in_mat4["uProjection"] = frame.proj_matrix;

        for (const auto& [entity_id, proxy] : frame.render_proxies)
        {
            if (!proxy.visible) continue;
            // pick only regular geometry
            if (!render::hasFlags(proxy.phases & render::RenderPhase::Opaque)) continue;

            const std::uint64_t v = static_cast<std::uint64_t>(entity_id);
            const std::uint32_t lo = static_cast<std::uint32_t>(v & 0xFFFF'FFFFull);
            const std::uint32_t hi = static_cast<std::uint32_t>(v >> 32);

            in.in_mat4["uModel"] = proxy.inputs.in_mat4.at("uModel");
            in.in_uint["uID_lo"] = lo;
            in.in_uint["uID_hi"] = hi;

            stats += co_await renderer.render(
                proxy.vertex_buffer,
                res.shader,
                in,
                opts,
                res.fbo
            );
        }
        co_return stats;
    }

}