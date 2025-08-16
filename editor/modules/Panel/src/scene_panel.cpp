#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "panel/scene_panel.hpp"

namespace astre::editor::panel
{
    // --- New: Modal state -----------------------------------------------------
    struct NewChunkModal {
        bool open = false;
        int x, y, z;
    } _new_chunk;

    struct NewEntityModal {
        bool open = false;
        world::ChunkID chunk;
        char name[128]{};
    } _new_entity;

    struct RenameEntityModal {
        bool                        open = false;
        world::ChunkID              chunk{};
        std::string                 old_name{};
        char                        name[128]{};
    } _rename_entity;

    struct ConfirmModal {
        bool                        open = false;
        std::string                 title{};
        std::string                 message{};
        std::function<void()>       on_confirm{};
    } _confirm;

    inline void _pushID(const world::ChunkID& c) noexcept {
        // use a compact, stable textual key (no allocations beyond small-string)
        ImGui::PushID(std::format("chunk:{}:{}:{}", c.x(), c.y(), c.z()).c_str());
    }
    inline void _pushID(const world::ChunkID& c, const ecs::EntityDefinition & entity_def) noexcept {
        ImGui::PushID(std::format("ent:{}:{}:{}:{}", c.x(), c.y(), c.z(), entity_def.id()).c_str());
    }
    inline void _pushID(ScenePanel::SelectedComponent kind) noexcept {
        ImGui::PushID(static_cast<int>(kind)); // enum is stable
    }

    void ScenePanel::updateSelectedEntity(controller::SelectionController & selection_controller)
    {
        if(!_pending_selection)return;

        if(_pending_selection->chunk_id && _pending_selection->entity_def)
        {
            selection_controller.setSelection(*_pending_selection->chunk_id, *_pending_selection->entity_def);
        }
        else if(_pending_selection->chunk_id)
        {
            selection_controller.setSelection(*_pending_selection->chunk_id);
        }
        else
        {
            selection_controller.deselect();
        }

        _pending_selection.reset(); 
    }

    bool ScenePanel::_addChunk(const world::ChunkID & id)
    {
        if(_chunk_entities_registry.mapping.contains(id))return false;

        // create empty chunk if not exists
        world::WorldChunk new_chunk{};
        new_chunk.mutable_id()->CopyFrom(id);
        
        _created_chunks.emplace(new_chunk);

        return true;
    }

    bool ScenePanel::_removeChunk(const world::ChunkID & id) noexcept
    {
        if(_chunk_entities_registry.mapping.contains(id) == false)return false;
        _removed_chunks.emplace(id);
        return true;
    }

    bool ScenePanel::_addEntity(const world::ChunkID & id, std::string_view name)
    {
        if(_chunk_entities_registry.mapping.contains(id) == false)return false;

        ecs::EntityDefinition new_entity{};
        new_entity.set_name(name);
        
        _created_entities[id].emplace(new_entity);        
        return true;
    }

    bool ScenePanel::_renameEntity(const world::ChunkID & id, const ecs::EntityDefinition & entity_def, std::string_view new_name)
    {
        return false;
    }

    bool ScenePanel::_removeEntity(const world::ChunkID & id, const ecs::EntityDefinition & entity_def) noexcept
    {
        _removed_entities[id].emplace(entity_def);
        return true;
    }

    bool ScenePanel::_addComponent(const world::ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, SelectedComponent component)
    {
        if (component == SelectedComponent::None) return false;
        
        ecs::EntityDefinition pending_entity_def = entity_def;

        switch(component)
        {
            case SelectedComponent::Transform:
                pending_entity_def.mutable_transform()->CopyFrom(ecs::TransformComponent{});
                break;

            case SelectedComponent::Health:
                pending_entity_def.mutable_health()->CopyFrom(ecs::HealthComponent{});
                break;

            case SelectedComponent::Visual:
                pending_entity_def.mutable_visual()->CopyFrom(ecs::VisualComponent{});
                break;

            case SelectedComponent::Input:
                pending_entity_def.mutable_input()->CopyFrom(ecs::InputComponent{});
                break;

            case SelectedComponent::Camera:
                pending_entity_def.mutable_camera()->CopyFrom(ecs::CameraComponent{});
                break;
            
            case SelectedComponent::Terrain:
                pending_entity_def.mutable_terrain()->CopyFrom(ecs::TerrainComponent{});
                pending_entity_def;
            
            case SelectedComponent::Light:
                pending_entity_def.mutable_light()->CopyFrom(ecs::LightComponent{});
                break;
            
            case SelectedComponent::Script:
                pending_entity_def.mutable_script()->CopyFrom(ecs::ScriptComponent{});
                break;

            default:
                spdlog::error("Unsupported component: {}", (int)component);
                return false;
        }

        _updated_entities[chunk_id].emplace(pending_entity_def);

        return true;
    }

    bool ScenePanel::_removeComponent(const world::ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, SelectedComponent component) noexcept
    {
        if (component == SelectedComponent::None) return false;

        ecs::EntityDefinition pending_entity_def = entity_def;

        switch(component)
        {
            case SelectedComponent::Transform:
                pending_entity_def.clear_transform();
                break;

            case SelectedComponent::Health:
                pending_entity_def.clear_health();
                break;

            case SelectedComponent::Visual:
                pending_entity_def.clear_visual();
                break;

            case SelectedComponent::Input:
                pending_entity_def.clear_input();
                break;

            case SelectedComponent::Camera:
                pending_entity_def.clear_camera();
                break;
            
            case SelectedComponent::Terrain:
                pending_entity_def.clear_terrain();
                break;
            
            case SelectedComponent::Light:
                pending_entity_def.clear_light();
                break;

            case SelectedComponent::Script:
                pending_entity_def.clear_script();
                break;
            
            default:
                spdlog::error("Unsupported component: {}", (int)component);
                return false;
        }

        _updated_entities[chunk_id].emplace(pending_entity_def);

        return true;
    }

    void ScenePanel::_drawAddComponentCombo(const world::ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        const std::string key      = std::format("addcmp:{}:{}:{}:{}", chunk_id.x(), chunk_id.y(), chunk_id.z(), entity_def.id());
        const std::string combo_id = std::format("##{}", key);

        // preview + pending come from your previous step
        auto& preview = _add_component_preview[key];
        if (preview.empty()) preview = "Add component...";
        auto& pending = _pending_component[key];

        // --- compute widths so combo doesn't eat the whole line ---
        const ImGuiStyle& style = ImGui::GetStyle();
        const float plus_w   = ImGui::CalcTextSize("+").x + style.FramePadding.x * 2.0f;
        const float spacing  = style.ItemInnerSpacing.x;
        const float avail_w  = ImGui::GetContentRegionAvail().x;
        const float min_w    = 50.0f;
        const float combo_w  = std::max(min_w, avail_w - plus_w - spacing);

        ImGui::SetNextItemWidth(combo_w);
        if (ImGui::BeginCombo(combo_id.c_str(), preview.c_str()))
        {
            auto item = [&](const char* label, SelectedComponent kind, bool has){
                if (has) return;
                const bool is_sel = (pending == kind);
                if (ImGui::Selectable(label, is_sel)) {
                    pending = kind;
                    preview = label; // show picked component on the bar
                }
            };

            item("Transform", SelectedComponent::Transform, entity_def.has_transform());
            item("Health",    SelectedComponent::Health,    entity_def.has_health());
            item("Visual",    SelectedComponent::Visual,    entity_def.has_visual());
            item("Input",     SelectedComponent::Input,     entity_def.has_input());
            item("Camera",    SelectedComponent::Camera,    entity_def.has_camera());
            item("Terrain",   SelectedComponent::Terrain,   entity_def.has_terrain());
            item("Light",     SelectedComponent::Light,     entity_def.has_light());
            item("Script",    SelectedComponent::Script,    entity_def.has_script());

            ImGui::EndCombo();
        }

        // render "+" on the same line with default spacing
        ImGui::SameLine(0.0f, spacing);
        const bool can_add = (pending != SelectedComponent::None);
        ImGui::BeginDisabled(!can_add);
        if (ImGui::SmallButton("+"))
        {
            if (!_addComponent(chunk_id, entity_def, pending)) 
            {
                spdlog::error("Failed to add component");
            }

            preview = "Add component...";
            pending = SelectedComponent::None;
        }
        ImGui::EndDisabled();
    }

    void ScenePanel::_drawComponent(
        bool has,
        std::string label,
        const world::ChunkID & chunk_id,
        const ecs::EntityDefinition & entity_def,
        SelectedComponent component)
    {
        if (!has) return;

        ImGui::BulletText("%s", label.c_str());

        // Right-click: remove component
        if (ImGui::BeginPopupContextItem(std::format("##cmp_ctx_{}_{}_{}_{}", 
        (int)chunk_id.x(), (int)chunk_id.y(), (int)chunk_id.z(),
             label).c_str())) {
            if (ImGui::MenuItem("Remove component")) 
            {
                if (!_removeComponent(chunk_id, entity_def, component))
                {
                    spdlog::error("[scene-panel] Failed to remove component");
                }
            }
            ImGui::EndPopup();
        }
    }

    void ScenePanel::draw(const model::DrawContext& ctx) noexcept {
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
            _pending_selection = Selection();
        }
        
        ImGui::TextUnformatted("Scene Hierarchy");
        ImGui::Separator();

        const bool world_open = ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);

        // Right-click on "World" -> add chunk
        if (ImGui::BeginPopupContextItem("##world_ctx")) {
            if (ImGui::MenuItem("Add chunk...")) {
                _new_chunk = {};
                _new_chunk.open = true;
            }
            ImGui::EndPopup();
        }

        if (world_open) 
        {
            // for every chunk we create a node
            for(auto & [chunk_id, entity_defs] : _chunk_entities_registry.mapping)
            {
                _pushID(chunk_id);

                const std::string chunk_label = std::format("Chunk ({},{},{})###chunk_{}_{}_{}",
                                                            chunk_id.x(), chunk_id.y(), chunk_id.z(),
                                                            chunk_id.x(), chunk_id.y(), chunk_id.z());

                ImGuiTreeNodeFlags chunk_node_flags = ImGuiTreeNodeFlags_SpanFullWidth;

                // chunk selected
                if (ctx.selection_controller.isChunkSelected(chunk_id))
                {
                    chunk_node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                const bool chunk_open = ImGui::TreeNodeEx(chunk_label.c_str(), chunk_node_flags);

                // Clicking the chunk row selects it
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
                {
                    _pending_selection = Selection{.chunk_id = chunk_id};
                }

                // Right-click chunk node
                if (ImGui::BeginPopupContextItem(std::format("##chunk_ctx_{}_{}_{}", chunk_id.x(), chunk_id.y(), chunk_id.z()).c_str())) {
                    if (ImGui::MenuItem("Add entity...")) {
                        _new_entity = {};
                        _new_entity.open = true;
                        _new_entity.chunk = chunk_id;
                        _new_entity.name[0] = '\0';
                    }
                    if (ImGui::MenuItem("Remove chunk...")) {
                        _confirm = {};
                        _confirm.open = true;
                        _confirm.title = "Remove Chunk";
                        _confirm.message = std::format("Do you want to remove chunk ({},{},{}) ? This operation cannot be undone.", chunk_id.x(), chunk_id.y(), chunk_id.z());
                        _confirm.on_confirm = [this, &chunk_id](){
                            _removeChunk(chunk_id);
                        };
                    }
                    ImGui::EndPopup();
                }

                if (chunk_open)
                {                        
                    // then for every entity in chunk
                    for(auto & [_, entity_def] : entity_defs)
                    {
                        _pushID(chunk_id, entity_def);

                        // Selected row highlight for the entity itself (no component selected)
                        ImGuiTreeNodeFlags entity_node_flags = ImGuiTreeNodeFlags_SpanFullWidth;

                        if (ctx.selection_controller.isEntitySelected(entity_def))
                        {
                            entity_node_flags |= ImGuiTreeNodeFlags_Selected;
                        }

                        bool open_entity_row = ImGui::TreeNodeEx(entity_def.name().c_str(), entity_node_flags);

                        // Clicking the entity row selects the entity (component=None)
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
                        {
                            _pending_selection = Selection{.chunk_id = chunk_id, .entity_def = entity_def, .component = SelectedComponent::None};
                        }

                        // Right-click entity row
                        if (ImGui::BeginPopupContextItem(std::format("##entity_ctx_{}_{}_{}_{}", chunk_id.x(), chunk_id.y(), chunk_id.z(), entity_def.id()).c_str())) {
                            if (ImGui::MenuItem("Rename...")) {
                                _rename_entity = {};
                                _rename_entity.open = true;
                                _rename_entity.chunk = chunk_id;
                                //_rename_entity.old_name = name;
                                std::snprintf(_rename_entity.name, sizeof(_rename_entity.name), "%s", entity_def.name().c_str());
                            }
                            if (ImGui::MenuItem("Delete...")) {
                                _confirm = {};
                                _confirm.open = true;
                                _confirm.title = "Delete Entity";
                                _confirm.message = std::format("Do you want to delete entity '{}' ?", entity_def.name());
                                _confirm.on_confirm = [this, &chunk_id, &entity_def](){
                                    _removeEntity(chunk_id, entity_def);
                                };
                            }
                            ImGui::EndPopup();
                        }

                        if(open_entity_row)
                        {
                            _drawAddComponentCombo(chunk_id, entity_def);

                            ImGui::Separator();
                            
                            _pushID(SelectedComponent::Transform);
                            _drawComponent(entity_def.has_transform(), "TransformComponent", chunk_id, entity_def, SelectedComponent::Transform);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Health);
                            _drawComponent(entity_def.has_health(),    "HealthComponent", chunk_id, entity_def,    SelectedComponent::Health);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Visual);
                            _drawComponent(entity_def.has_visual(),    "VisualComponent", chunk_id, entity_def,    SelectedComponent::Visual);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Input);
                            _drawComponent(entity_def.has_input(),     "InputComponent", chunk_id, entity_def,     SelectedComponent::Input);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Camera);
                            _drawComponent(entity_def.has_camera(),    "CameraComponent", chunk_id, entity_def,    SelectedComponent::Camera);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Terrain);
                            _drawComponent(entity_def.has_terrain(),   "TerrainComponent", chunk_id, entity_def,   SelectedComponent::Terrain);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Light);
                            _drawComponent(entity_def.has_light(),     "LightComponent", chunk_id, entity_def,     SelectedComponent::Light);
                            ImGui::PopID();

                            _pushID(SelectedComponent::Script);
                            _drawComponent(entity_def.has_script(),    "ScriptComponent", chunk_id, entity_def,    SelectedComponent::Script);
                            ImGui::PopID();

                            ImGui::Separator();

                            ImGui::TreePop();
                        }

                        ImGui::PopID();
                    }

                    // pop chunk node
                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            // pop world node
            ImGui::TreePop();
        }
        
        // --- Modals -----------------------------------------------------------
        if (_new_chunk.open) ImGui::OpenPopup("Add Chunk");
        if (ImGui::BeginPopupModal("Add Chunk", &_new_chunk.open, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            ImGui::InputInt("X", &_new_chunk.x);
            ImGui::InputInt("Y", &_new_chunk.y);
            ImGui::InputInt("Z", &_new_chunk.z);
            if (ImGui::Button("Create")) 
            {
                world::ChunkID id;
                id.set_x(_new_chunk.x);
                id.set_y(_new_chunk.y);
                id.set_z(_new_chunk.z);
                _addChunk(id);
                _new_chunk.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                _new_chunk.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Add entity
        if (_new_entity.open) ImGui::OpenPopup("Add Entity");
        if (ImGui::BeginPopupModal("Add Entity", &_new_entity.open, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Name", _new_entity.name, sizeof(_new_entity.name));
            if (ImGui::Button("Create")) {
                _addEntity(_new_entity.chunk, _new_entity.name);
                _new_entity.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                _new_entity.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Rename entity
        if (_rename_entity.open) ImGui::OpenPopup("Rename Entity");
        if (ImGui::BeginPopupModal("Rename Entity", &_rename_entity.open, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("New name", _rename_entity.name, sizeof(_rename_entity.name));
            if (ImGui::Button("Rename")) {
                //_renameEntity(_rename_entity.chunk, _rename_entity.old_name, _rename_entity.name);
                _rename_entity.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                _rename_entity.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Confirm modal (generic)
        constexpr float confirm_modal_min_width  = 520.0f;
        constexpr float confirm_text_wrap_width  = 520.0f;

        if (_confirm.open) ImGui::OpenPopup("Confirm");

        // Optional min width for the modal (keeps auto-resize behavior for height).
        ImGui::SetNextWindowSizeConstraints(ImVec2(confirm_modal_min_width, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        if (ImGui::BeginPopupModal("Confirm", &_confirm.open, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(_confirm.title.c_str());
            ImGui::Separator();

            // force text to wrap at a wider pixel width (relative to current cursor X).
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + confirm_text_wrap_width);
            ImGui::TextUnformatted(_confirm.message.c_str());
            ImGui::PopTextWrapPos();

            if (ImGui::Button("OK")) {
                if (_confirm.on_confirm) _confirm.on_confirm();
                _confirm.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                _confirm.open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // --- Bottom toolbar -------------------------------------------------------
        ImGui::Separator();

        // Reserve height for one line of buttons
        const float toolbarHeight = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("##scene_toolbar", ImVec2(0, toolbarHeight), false, ImGuiWindowFlags_NoScrollbar);

        {        
            // Add Chunk button
            if (ImGui::SmallButton("+"))
            {
                _new_chunk = {};
                _new_chunk.open = true;
            }

            ImGui::SameLine();

            // Remove Chunk button
            ImGui::BeginDisabled(!(ctx.selection_controller.isAnyChunkSelected()));
            if (ImGui::SmallButton("-")) {
                const world::ChunkID chunk_id = ctx.selection_controller.getChunkSelection();

                _confirm = {};
                _confirm.open = true;
                _confirm.title = "Remove Chunk";
                _confirm.message = std::format("Do you want to remove chunk ({},{},{}) ? This operation cannot be undone.",
                                               chunk_id.x(),
                                               chunk_id.y(),
                                               chunk_id.z());
                _confirm.on_confirm = [this, chunk_id]() {
                    _removeChunk(chunk_id);
                };
            }
            ImGui::EndDisabled();
        
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(0.0f, 32.0f));
            ImGui::SameLine();

            // Add Entity button
            ImGui::BeginDisabled(!(ctx.selection_controller.isAnyChunkSelected()));
            if (ImGui::SmallButton("o")) { // Entity symbol
                const world::ChunkID chunk_id = ctx.selection_controller.getChunkSelection();
                
                _new_entity = {};
                _new_entity.open = true;
                _new_entity.chunk = chunk_id;
                _new_entity.name[0] = '\0';
            }
            ImGui::EndDisabled();
        
            ImGui::SameLine();
        
            // Remove Entity button
            ImGui::BeginDisabled(!(
                ctx.selection_controller.isAnyChunkSelected() &&
                ctx.selection_controller.isAnyEntitySelected()
            ));
            if (ImGui::SmallButton("x")) { // Entity remove symbol
                const world::ChunkID chunk_id = ctx.selection_controller.getChunkSelection();
                const ecs::EntityDefinition entity_def = ctx.selection_controller.getEntitySelection();

                _confirm = {};
                _confirm.open = true;
                _confirm.title = "Delete Entity";
                _confirm.message = std::format("Do you want to delete entity '{}' ?", entity_def.name());
                _confirm.on_confirm = [this, chunk_id, entity_def]() {
                    _removeEntity(chunk_id, entity_def);
                };
            }
            ImGui::EndDisabled();
        }

        ImGui::EndChild();

        ImGui::End();
    }
}