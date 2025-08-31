#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

#include "pipeline/deferred_shading.hpp"

namespace astre::pipeline
{
    struct DebugOverlayResources
    {
        std::size_t debug_overlay_shader;
    };

    asio::awaitable<std::optional<DebugOverlayResources>> buildDebugOverlayResources(render::IRenderer & renderer);

    asio::awaitable<render::FrameStats> renderDebugOverlay(
        render::IRenderer & renderer,
        const DebugOverlayResources & resources,
        const render::Frame & frame,
        std::optional<std::size_t> fbo = std::nullopt);
}