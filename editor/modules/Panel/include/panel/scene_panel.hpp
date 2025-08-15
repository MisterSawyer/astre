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
        ecs::Entity entity;
        SelectedComponent component{SelectedComponent::None};
    };

    inline bool operator==(const SelectedEntity& lhs, const SelectedEntity& rhs) noexcept 
    {
        return  lhs.chunk_id == rhs.chunk_id &&
                lhs.entity == rhs.entity &&
                lhs.component == rhs.component;
    }

    class ScenePanel final : public IPanel 
    {
    public:
        ScenePanel();

        std::string_view name() const noexcept override { return "Scene"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

        std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> getSelectedEntityDef() const;

        void selectEntity(const ecs::Entity & entity);
        bool selectedEntityChanged() const noexcept;
        void resetSelectedEntityChanged() noexcept;

        const absl::flat_hash_set<world::WorldChunk> & getCreatedChunks() const noexcept { return _created_chunks; }
        const absl::flat_hash_set<world::ChunkID> & getRemovedChunks() const noexcept { return _removed_chunks; }

        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> & getCreatedEntities() { return _created_entities; }
        const absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> & getRemovedEntities() const noexcept { return _removed_entities; }
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> & getUpdatedEntities() { return _updated_entities; }

        void resetEvents() noexcept 
        {
            _created_chunks.clear();
            _removed_chunks.clear();
            
            _created_entities.clear();
            _removed_entities.clear();
            _updated_entities.clear();
        }

        void loadEntitesDefs(world::WorldStreamer & world_streamer);

    private:

        void _drawComponent(bool has, std::string label, const world::ChunkID & chunk_id, const ecs::Entity & entity, SelectedComponent kind);

        bool _addChunk(const world::ChunkID & id);
        bool _removeChunk(const world::ChunkID & id) noexcept;

        bool _addEntity(const world::ChunkID & id, std::string_view name);
        bool _renameEntity(const world::ChunkID & id, const ecs::Entity & entity, std::string_view new_name);
        bool _removeEntity(const world::ChunkID & id, const ecs::Entity & entity) noexcept;

        bool _addComponent(const world::ChunkID & id, const ecs::Entity & entity, SelectedComponent component);
        bool _removeComponent(const world::ChunkID & id, const ecs::Entity & entity, SelectedComponent component) noexcept;
        void _drawAddComponentCombo(const world::ChunkID & chunk_id, ecs::EntityDefinition & entity_def);

        bool _visible{true};

        // Selection state
        bool _selected_entity_changed{false};
        std::optional<SelectedEntity> _selection;

        absl::flat_hash_map<world::ChunkID, absl::flat_hash_map<ecs::Entity, ecs::EntityDefinition>> _chunk_entities_defs;

        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _created_entities;
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _updated_entities;
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _removed_entities;

        absl::flat_hash_set<world::WorldChunk> _created_chunks;
        absl::flat_hash_set<world::ChunkID> _removed_chunks;


        absl::flat_hash_map<std::string, std::string> _add_component_preview;
        absl::flat_hash_map<std::string, SelectedComponent> _pending_component;
    };

}
