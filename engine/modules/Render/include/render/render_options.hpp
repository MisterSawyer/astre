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
        //Textured,
        //Shadow,
        //ShadowTextured,
        _COUNT // must be last, special value used to determine number of modes
    };

    /**
     * @brief Primitive topology used for the draw call.
     */
    enum class PrimitiveTopology
    {
        Triangles,
        Lines // index buffer is a line list (edge-only meshes, e.g. wire_cube_prefab)
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
        std::optional<PolygonOffset> polygon_offset = std::nullopt;
        bool write_depth = true;
        bool depth_test = true; // set false to draw always-on-top (e.g. debug overlays)
        PrimitiveTopology topology = PrimitiveTopology::Triangles;
    };
}