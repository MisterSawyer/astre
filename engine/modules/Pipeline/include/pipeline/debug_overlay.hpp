#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

#include "asset/world_streamer.hpp"

#include "pipeline/deferred_shading.hpp"

namespace astre::pipeline
{
    struct DebugOverlayResources
    {
        std::size_t debug_overlay_shader;
        // carries the shared wire cube vb + line render options, chosen the same
        // way SelectionOverlayController picks its model via a RenderProxy.
        render::RenderProxy chunk_border_proxy;
    };

    // Draw an edge-only wire cube per chunk, colored by streaming state, using the
    // shared `wire_cube_prefab` and the debug shader. Chunks are drawn state by
    // state in priority order (Unloaded -> Required -> Loaded -> ToReload -> Dirty)
    // so that on edges shared by adjacent chunks the higher-priority state's color
    // wins (drawn last, on top). Borders are static, so this bypasses the render
    // proxy / interpolation path entirely.
    asio::awaitable<render::FrameStats> renderChunkBorders(
        render::IRenderer & renderer,
        const DebugOverlayResources & resources,
        const asset::WorldStreamer & world_streamer,
        const math::Mat4 & view,
        const math::Mat4 & proj,
        std::optional<std::size_t> fbo = std::nullopt);

    asio::awaitable<std::optional<DebugOverlayResources>> buildDebugOverlayResources(render::IRenderer & renderer);

    asio::awaitable<render::FrameStats> renderDebugOverlay(
        render::IRenderer & renderer,
        const DebugOverlayResources & resources,
        const render::Frame & frame,
        std::optional<std::size_t> fbo = std::nullopt);
}