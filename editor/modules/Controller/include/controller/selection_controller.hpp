#pragma once

#include "ecs/ecs.hpp"
#include "file/file.hpp"

namespace astre::editor::controller
{
    class SelectionController
    {
    public:
        SelectionController() = default;
        ~SelectionController() = default;

        bool isAnyEntitySelected() const noexcept { return _entity_selection.has_value(); }
        bool isEntitySelected(const proto::ecs::EntityDefinition & entity_def) const noexcept 
        { 
            if(_entity_selection.has_value() && _entity_selection.value() == entity_def) return true;
            return false;
        }

        bool isAnyChunkSelected() const noexcept { return _chunk_selection.has_value(); }
        bool isChunkSelected(const file::ChunkID & chunk_id) const noexcept 
        {
            if(_chunk_selection.has_value() && _chunk_selection.value() == chunk_id) return true;
            return false;
        }

        const proto::ecs::EntityDefinition & getEntitySelection() const noexcept { return *_entity_selection; }
        const file::ChunkID & getChunkSelection() const noexcept { return *_chunk_selection; }

        void updateSelectedEntity(proto::ecs::EntityDefinition entity_def) noexcept { _entity_selection = entity_def; _entity_updated = true; }
        bool selectedEntityUpdated()
        {
           return _entity_updated;
        }
        void clearSelectedEntityUpdate() noexcept { _entity_updated = false; }

        void setSelection(file::ChunkID chunk_id) noexcept
        {
            _chunk_selection = chunk_id;
            _entity_selection = std::nullopt;
        }

        void setSelection(file::ChunkID chunk_id, proto::ecs::EntityDefinition entity_def) noexcept
        {
            _chunk_selection = chunk_id;
            _entity_selection = entity_def;
        }

        void deselect()
        {
            _chunk_selection = std::nullopt;
            _entity_selection = std::nullopt;
        }


    private:
        std::optional<file::ChunkID> _chunk_selection;
        std::optional<proto::ecs::EntityDefinition> _entity_selection;
        bool _entity_updated = false;

    };
}