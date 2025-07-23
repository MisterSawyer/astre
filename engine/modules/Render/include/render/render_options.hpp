#pragma once

#include <optional>

namespace astre::render
{   
    /**
     * @brief Enum for render modes
     * 
     */
    enum class RenderMode
    {
        Wireframe,
        Solid,
        Textured,
        Shadow,
        ShadowTextured,
        Count
    };

    /**
     * @brief Struct for polygon offset
     * 
     */
    struct PolygonOffset
    {
        float factor;
        float units;
    };

    /**
     * @brief Struct for render options
     * 
     */
    struct RenderOptions
    {
        RenderMode mode = RenderMode::Solid;
        std::optional<PolygonOffset> polygon_offset;
    };
}