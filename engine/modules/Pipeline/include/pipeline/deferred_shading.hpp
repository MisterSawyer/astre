#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

namespace astre::pipeline
{
    // const and where
    // are valid during all frames 
    struct DeferredShadingResources
    {
        std::size_t deferred_fbo; // where
        std::size_t shadow_map_shader; // const
        std::vector<std::size_t> deferred_textures; // where

        std::size_t light_ssbo;

        std::vector<std::size_t> shadow_map_fbos; // where
        std::vector<std::size_t> shadow_map_textures; // where

        std::size_t screen_quad_vb; // where
        std::size_t screen_quad_shader; // const
    };

    asio::awaitable<DeferredShadingResources> buildDeferredShadingResources(render::IRenderer & renderer);

    asio::awaitable<render::FrameStats> deferredShadingStage(
        render::IRenderer & renderer,
        const DeferredShadingResources & resources,
        float alpha, const render::Frame & prev, const render::Frame & curr,
        const render::RenderOptions & gbuffer_render_options,
        const render::RenderOptions & shadow_map_render_options,
        std::optional<std::size_t> fbo = std::nullopt);
}