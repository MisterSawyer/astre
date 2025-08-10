#pragma once
#include <string_view>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "ecs/ecs.hpp"
#include "world/world.hpp"

#include "panel/panel_interface.hpp"

namespace astre::editor::panel 
{
    enum class SelectedComponent : uint8_t 
    {
        None, Transform, Health, Visual, Input, Camera, Terrain, Light, Script
    };

    struct SelectedEntity 
    {
        world::ChunkID chunk_id;
        std::string entity_name;
        SelectedComponent component{SelectedComponent::None};
    };

    inline bool operator==(const SelectedEntity& lhs, const SelectedEntity& rhs) noexcept 
    {
        return  lhs.chunk_id == rhs.chunk_id &&
                lhs.entity_name == rhs.entity_name &&
                lhs.component == rhs.component;
    }

    class ScenePanel final : public IPanel 
    {
    public:
        ScenePanel(world::WorldStreamer & world_streamer);

        std::string_view name() const noexcept override { return "Scene"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

        std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> getSelectedEntityDef() const;
        bool selectedEntityChanged() const noexcept;
        void resetSelectedEntityChanged() noexcept;

    private:
        void _loadEntitesDefs();

        void _drawComponent(bool has, std::string label, world::ChunkID chunk_id, std::string entity_name, SelectedComponent kind);

        world::WorldStreamer & _world_streamer;

        bool _visible{true};

        // Selection state
        bool _selected_entity_changed{false};
        std::optional<SelectedEntity> _selection;

        absl::flat_hash_map<world::ChunkID, absl::flat_hash_map<std::string, ecs::EntityDefinition>> _chunk_entities_defs;
    };

}
