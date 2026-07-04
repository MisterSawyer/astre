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
        }
        resources.debug_overlay_shader = *debug_shader_res;

        const auto cube_vb_res = renderer.getVertexBuffer("wire_cube_prefab");
        if (!cube_vb_res) {
            spdlog::warn("No 'wire_cube_prefab' vertex buffer; debug phase will skip.");
            co_return std::nullopt;
        }
        resources.chunk_border_proxy = render::RenderProxy
        {
            .visible = true,
            .phases = render::RenderPhase::Debug,
            .vertex_buffer = *cube_vb_res,
            .options = render::RenderOptions
            {
                .mode = render::RenderMode::Solid, // topology=Lines draws edges; no polygon-mode needed
                .write_depth = false,
                .depth_test = false, // chunk borders draw on top; camera sits inside the box
                .topology = render::PrimitiveTopology::Lines, // edge-only, and lines aren't face-culled
            },
        };

        co_return resources;
    }

    static math::Vec4 chunkStateColor(asset::WorldStreamer::ChunkDebugState state)
    {
        using S = asset::WorldStreamer::ChunkDebugState;
        switch (state)
        {
            case S::Dirty:    return math::Vec4(1.0f, 0.0f, 0.0f, 1.0f);    // red
            case S::ToReload: return math::Vec4(1.0f, 1.0f, 0.0f, 1.0f);    // yellow
            case S::Loaded:   return math::Vec4(1.0f, 1.0f, 1.0f, 1.0f);    // white
            case S::Required: return math::Vec4(0.0f, 1.0f, 0.0f, 1.0f);    // green
            case S::Unloaded: return math::Vec4(0.1f, 0.1f, 0.1f, 1.0f); // gray (distinct from white loaded)
        }
        return math::Vec4(1.0f, 0.0f, 1.0f, 1.0f); // unreachable; magenta = bug
    }

    asio::awaitable<render::FrameStats> renderChunkBorders(
        render::IRenderer & renderer,
        const DebugOverlayResources & resources,
        const asset::WorldStreamer & world_streamer,
        const math::Mat4 & view,
        const math::Mat4 & proj,
        std::optional<std::size_t> fbo)
    {
        render::FrameStats stats;
        if (!resources.debug_overlay_shader) co_return stats;

        const render::RenderProxy & proxy = resources.chunk_border_proxy;

        const auto states = co_await world_streamer.snapshotChunkStates();
        const float s = world_streamer.chunkSize();

        using S = asset::WorldStreamer::ChunkDebugState;
        // Enum order is already the desired priority; draw low -> high so higher
        // priority overwrites shared edges (depth_test off, last write wins).
        constexpr S order[] = { S::Unloaded, S::Required, S::Loaded, S::ToReload, S::Dirty };

        render::ShaderInputs inputs;
        inputs.in_mat4["uView"] = view;
        inputs.in_mat4["uProjection"] = proj;

        // O(states * chunks) rescans; trivial for debug chunk counts.
        for (const S draw_state : order)
        {
            inputs.in_vec4["uColor"] = chunkStateColor(draw_state);
            for (const auto & [id, state] : states)
            {
                if (state != draw_state) continue;

                const math::Vec3 center{(id.x() + 0.5f) * s, (id.y() + 0.5f) * s, (id.z() + 0.5f) * s};
                inputs.in_mat4["uModel"] =
                    math::translate(glm::mat4(1.0f), center) *
                    math::scale(glm::mat4(1.0f), math::Vec3(s));

                stats += co_await renderer.render(proxy.vertex_buffer, resources.debug_overlay_shader, inputs, proxy.options, fbo);
            }
        }

        co_return stats;
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