#pragma once

#include <utility>
#include <optional>

#include "math/math.hpp"
#include "render/render.hpp"
#include "ecs/ecs.hpp"
#include "world/world.hpp"

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

        float logic_fps{0.0f};
        float logic_frame_time{0.0f};
        astre::render::FrameStats stats;

        math::Vec3 camera_position;
        
        bool selected_entity_updated;
        std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> selected_entity;
    };
}