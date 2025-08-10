#include <format>

#include <imgui.h>

#include "panel/scene_panel.hpp"

namespace astre::editor::panel
{
    ScenePanel::ScenePanel(world::WorldStreamer & world_streamer)
    : _world_streamer(world_streamer)
    {
        loadEntitesDefs();
    }

    void ScenePanel::loadEntitesDefs()
    {
        _chunk_entities_defs.clear();
        
        for(const auto & chunk_id : _world_streamer.getAllChunks())
        {
            // then for every node we obtain chunk definition
            auto chunk_res = _world_streamer.readChunk(chunk_id, asset::use_json);
                        
            if (chunk_res.has_value())
            {
                const auto & chunk = *chunk_res;

                // then for every entity in chunk
                for(const auto & entity_def : chunk.entities())
                {
                    _chunk_entities_defs[chunk_id].emplace(entity_def.name(), entity_def);
                }
            }
        }
    }

    std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> ScenePanel::getSelectedEntityDef() const
    {
        if (_selection)
        {
            return std::make_pair(_selection->chunk_id, _chunk_entities_defs.at(_selection->chunk_id).at(_selection->entity_name));
        }
        return std::nullopt;
    }

    bool ScenePanel::selectedEntityChanged() const noexcept
    {
        return _selected_entity_changed;
    }

    void ScenePanel::resetSelectedEntityChanged() noexcept 
    { 
        _selected_entity_changed = false;
    }

    void ScenePanel::_drawComponent(bool has, std::string label, world::ChunkID chunk_id, std::string entity_name, SelectedComponent kind)
    {
        if (!has) return;

        // Use Selectable for a clean selection UX
        ImGuiSelectableFlags sflags = ImGuiSelectableFlags_AllowDoubleClick;

        bool selected = _selection &&
                _selection->chunk_id == chunk_id &&
                _selection->entity_name == entity_name &&
                _selection->component == kind;

        if (ImGui::Selectable(label.c_str(), selected, sflags)) 
        {
            if(!_selection || (_selection && (_selection->chunk_id != chunk_id || _selection->entity_name != entity_name)))
            {
                _selected_entity_changed = true;
            }

            // Selecting a component auto-selects the entity as requested
            _selection = SelectedEntity{std::move(chunk_id), std::move(entity_name), kind};
        }
    }

    void ScenePanel::draw(const panel::DrawContext& ctx) noexcept {
        if (!_visible) return;

        // dock into the node
        if (ctx.scene_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.scene_dock_id, ImGuiCond_Always);

        // No close button => cannot be closed by the user
        ImGuiWindowFlags flags =    
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar;
        
        if (!ImGui::Begin("Scene", &_visible, flags)) { ImGui::End(); return; }
        
        // If we click in empty space inside the Scene window -> deselect
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            _selection.reset();
            _selected_entity_changed = true;
        }
        
        ImGui::TextUnformatted("Scene Hierarchy");
        ImGui::Separator();

        if (ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) 
        {
            // for every chunk we create a node
            for(const auto & [chunk_id, entity_defs] : _chunk_entities_defs)
            {
                if (ImGui::TreeNode(std::format("Chunk ({},{},{})", chunk_id.x(), chunk_id.y(), chunk_id.z()).c_str())) 
                {                        
                    // then for every entity in chunk
                    for(const auto & [name, entity_def] : entity_defs)
                    {
                        // Selected row highlight for the entity itself (no component selected)
                        ImGuiTreeNodeFlags entity_node_flags = ImGuiTreeNodeFlags_SpanFullWidth;

                        if (_selection &&
                            _selection->chunk_id == chunk_id &&
                            _selection->entity_name == name)
                        {
                            entity_node_flags |= ImGuiTreeNodeFlags_Selected;
                        }

                        bool open_entity_row = ImGui::TreeNodeEx(name.c_str(), entity_node_flags);

                        // Clicking the entity row selects the entity (component=None)
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
                        {
                            if(!_selection || (_selection && (_selection->chunk_id != chunk_id || _selection->entity_name != name)))
                            {
                                _selected_entity_changed = true;
                            }

                            _selection = SelectedEntity{chunk_id, name, SelectedComponent::None};
                        }

                        if(open_entity_row)
                        {
                            _drawComponent(entity_def.has_transform(), "TransformComponent", chunk_id, name, SelectedComponent::Transform);
                            _drawComponent(entity_def.has_health(),    "HealthComponent", chunk_id, name,    SelectedComponent::Health);
                            _drawComponent(entity_def.has_visual(),    "VisualComponent", chunk_id, name,    SelectedComponent::Visual);
                            _drawComponent(entity_def.has_input(),     "InputComponent", chunk_id, name,     SelectedComponent::Input);
                            _drawComponent(entity_def.has_camera(),    "CameraComponent", chunk_id, name,    SelectedComponent::Camera);
                            _drawComponent(entity_def.has_terrain(),   "TerrainComponent", chunk_id, name,   SelectedComponent::Terrain);
                            _drawComponent(entity_def.has_light(),     "LightComponent", chunk_id, name,     SelectedComponent::Light);
                            _drawComponent(entity_def.has_script(),    "ScriptComponent", chunk_id, name,    SelectedComponent::Script);
                            
                            ImGui::TreePop();
                        }
                    }

                    // pop chunk node
                    ImGui::TreePop();
                }
            }
            // pop world node
            ImGui::TreePop();
        }
        
        ImGui::End();
    }
}