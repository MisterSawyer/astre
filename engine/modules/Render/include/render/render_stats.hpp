#pragma once

#include <utility>

namespace astre::render
{   
    struct FrameStats 
    {
        std::uint32_t draw_calls = 0;
        std::uint32_t vertices = 0;
        std::uint32_t triangles = 0;
        // optionally: shader switches, FBO binds, texture binds, etc.
    };

}