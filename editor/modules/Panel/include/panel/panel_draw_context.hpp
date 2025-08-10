#pragma once

namespace astre::editor::panel
{
    struct DrawContext
    {
        float dt{0.0f};
        unsigned int scene_dock_id{0};
        unsigned int properties_dock_id{0};
        unsigned int assets_dock_id{0};
        unsigned int viewport_dock_id{0};

        std::size_t viewport_texture;
    };
}