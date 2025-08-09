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

        inline FrameStats & operator+=(const FrameStats & rhs) {
            draw_calls += rhs.draw_calls;
            vertices += rhs.vertices;
            triangles += rhs.triangles;
            return *this;
        }
    };

    inline FrameStats operator+(const FrameStats & lhs, const FrameStats & rhs) 
    { 
        return {    
            lhs.draw_calls + rhs.draw_calls,
            lhs.vertices + rhs.vertices,
            lhs.triangles + rhs.triangles
        }; 
    }

}