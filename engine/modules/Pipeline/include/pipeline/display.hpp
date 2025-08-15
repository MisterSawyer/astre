#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

namespace astre::pipeline
{
    struct DisplayResources
    {
        std::pair<unsigned,unsigned> size;
        float aspect = 1.0f;
        std::size_t viewport_fbo = 0;
    };

    asio::awaitable<std::optional<DisplayResources>> buildDisplayResources(render::IRenderer & renderer, std::pair<unsigned,unsigned> size); 
}