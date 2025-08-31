#include "pipeline/renderer_state.hpp"

namespace astre::pipeline
{
    asio::awaitable<std::optional<RendererState>> buildRendererState(render::IRenderer & renderer, std::pair<unsigned,unsigned> display_size)
    {
        // Create viewport FBO
        auto display_resources_res = co_await pipeline::buildDisplayResources(renderer, display_size);
        if(!display_resources_res) 
        {
            spdlog::error("Failed to create display resources");
            co_return std::nullopt;
        }

        // Create deferred shading FBO
        auto render_resources_res = co_await pipeline::buildDeferredShadingResources(renderer, display_resources_res->size);
        if(!render_resources_res)
        {
            spdlog::error("Failed to build deferred shading resources");
            co_return std::nullopt;
        }

        // Create debug overlay resources
        auto debug_overlay_resources_res = co_await pipeline::buildDebugOverlayResources(renderer);
        if(!debug_overlay_resources_res)
        {
            spdlog::error("Failed to build debug overlay resources");
            co_return std::nullopt;
        }

        // Get viewport FBO textures
        auto viewport_fbo_textures = renderer.getFrameBufferObjectTextures(display_resources_res->viewport_fbo);

        co_return RendererState{
            .display = std::move(*display_resources_res),
            .deferred_shading = std::move(*render_resources_res),
            .debug_overlay = std::move(*debug_overlay_resources_res),

            .viewport_fbo_textures = std::move(viewport_fbo_textures)
        };
    }
}