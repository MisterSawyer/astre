#pragma once
#include <string_view>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "file/file.hpp"
#include "ecs/ecs.hpp"

#include "panel/panel_interface.hpp"

#include "model/world_snapshot.hpp"
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

        ScenePanel(const model::WorldSnapshot & world_snapshot) 
        : _world_snapshot(world_snapshot) 
        {};

        std::string_view name() const noexcept override { return "Scene"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const model::DrawContext& ctx) noexcept override;

        void updateSelectedEntity(controller::SelectionController & selection_controller);

    private:
        struct Selection
        {
            std::optional<proto::file::ChunkID>           chunk_id;
            std::optional<proto::ecs::EntityDefinition>    entity_def;
            std::optional<SelectedComponent>        component;
        };

        bool _addChunk(const proto::file::ChunkID & id);
        bool _removeChunk(const proto::file::ChunkID & id) noexcept;

        bool _addEntity(const proto::file::ChunkID & id, std::string_view name);
        bool _renameEntity(const proto::file::ChunkID & id, const proto::ecs::EntityDefinition & entity_def, std::string_view new_name);
        bool _removeEntity(const proto::file::ChunkID & id, const proto::ecs::EntityDefinition & entity_def) noexcept;

        bool _addComponent(const proto::file::ChunkID & id, const proto::ecs::EntityDefinition & entity_def, SelectedComponent component);
        bool _removeComponent(const proto::file::ChunkID & id, const proto::ecs::EntityDefinition & entity_def, SelectedComponent component) noexcept;

        void _drawComponent(bool has, std::string label, const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def, SelectedComponent component);
        void _drawAddComponentCombo(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def);

        bool _visible{true};

        const model::WorldSnapshot & _world_snapshot;

        absl::flat_hash_map<std::string, std::string> _add_component_preview;
        absl::flat_hash_map<std::string, SelectedComponent> _pending_component;

        std::optional<Selection> _pending_selection;
    };

}
