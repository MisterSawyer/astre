#pragma once

#include "pipeline/deferred_shading.hpp"
#include "pipeline/debug_overlay.hpp"
#include "pipeline/display.hpp"

namespace astre::pipeline
{
    struct RendererState
    {
        const DisplayResources display;
        const DeferredShadingResources deferred_shading;
        const DebugOverlayResources debug_overlay;

        const std::vector<std::size_t> viewport_fbo_textures;
    };

    asio::awaitable<std::optional<RendererState>> buildRendererState(render::IRenderer & renderer, std::pair<unsigned,unsigned> display_size);
}