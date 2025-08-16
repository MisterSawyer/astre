#pragma once
#include <string_view>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "ecs/ecs.hpp"
#include "world/world.hpp"

#include "panel/panel_interface.hpp"

#include "model/chunk_entities_registry.hpp"
#include "controller/selection_controller.hpp"

namespace astre::editor::panel 
{
    class ScenePanel final : public IPanel 
    {
    public:
        enum class SelectedComponent : uint8_t 
        {
            None, Transform, Health, Visual, Input, Camera, Terrain, Light, Script
        };

        ScenePanel(const model::ChunkEntityRegistry & chunk_entities_registry) 
        : _chunk_entities_registry(chunk_entities_registry) 
        {};

        std::string_view name() const noexcept override { return "Scene"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const model::DrawContext& ctx) noexcept override;

        void updateSelectedEntity(controller::SelectionController & selection_controller);

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

    private:
        struct Selection
        {
            std::optional<world::ChunkID>           chunk_id;
            std::optional<ecs::EntityDefinition>    entity_def;
            std::optional<SelectedComponent>        component;
        };

        bool _addChunk(const world::ChunkID & id);
        bool _removeChunk(const world::ChunkID & id) noexcept;

        bool _addEntity(const world::ChunkID & id, std::string_view name);
        bool _renameEntity(const world::ChunkID & id, const ecs::EntityDefinition & entity_def, std::string_view new_name);
        bool _removeEntity(const world::ChunkID & id, const ecs::EntityDefinition & entity_def) noexcept;

        bool _addComponent(const world::ChunkID & id, const ecs::EntityDefinition & entity_def, SelectedComponent component);
        bool _removeComponent(const world::ChunkID & id, const ecs::EntityDefinition & entity_def, SelectedComponent component) noexcept;

        void _drawComponent(bool has, std::string label, const world::ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, SelectedComponent component);
        void _drawAddComponentCombo(const world::ChunkID & chunk_id, const ecs::EntityDefinition & entity_def);

        bool _visible{true};

        const model::ChunkEntityRegistry & _chunk_entities_registry;

        absl::flat_hash_set<world::WorldChunk> _created_chunks;
        absl::flat_hash_set<world::ChunkID> _removed_chunks;

        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _created_entities;
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _updated_entities;
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> _removed_entities;

        absl::flat_hash_map<std::string, std::string> _add_component_preview;
        absl::flat_hash_map<std::string, SelectedComponent> _pending_component;

        std::optional<Selection> _pending_selection;
    };

}
