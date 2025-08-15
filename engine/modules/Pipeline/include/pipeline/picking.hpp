#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

namespace astre::pipeline
{
    struct PickingResources
    {
        std::size_t fbo = 0;
        std::size_t shader = 0; // writes uEntityID to color0
        std::pair<unsigned,unsigned> size{};
    };

    asio::awaitable<std::optional<PickingResources>>
        buildPickingResources(render::IRenderer& renderer, std::pair<unsigned,unsigned> size);

    asio::awaitable<render::FrameStats>
        renderPickingIds(render::IRenderer& renderer, const render::Frame& frame,
                 const PickingResources& res);

}