#include <imgui.h>
#include <spdlog/spdlog.h>

#include "ecs/ecs.hpp"

#include "panel/properties_panel.hpp"

namespace astre::editor::panel 
{
    
    static bool _drawTransformComponent(ecs::TransformComponent & transform)
    {
        bool changed = false;

        float position[3] = {transform.position().x(), transform.position().y(), transform.position().z()};
        float rotation[4] = {transform.rotation().w(), transform.rotation().x(), transform.rotation().y(), transform.rotation().z()}; // {0.f, 0.f, 0.f};
        float scale[3]    = {transform.scale().x(), transform.scale().y(), transform.scale().z()};

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            ImGui::TextUnformatted("Position");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat3("##pos", position, 0.1f);
            ImGui::PopItemWidth();

            ImGui::TextUnformatted("Rotation");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat4("##rot", rotation, 0.1f);
            ImGui::PopItemWidth();

            ImGui::TextUnformatted("Scale");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat3("##scale", scale, 0.1f);
            ImGui::PopItemWidth();

            // apply changes
            if(changed)
            {
                transform.mutable_position()->set_x(position[0]);
                transform.mutable_position()->set_y(position[1]);
                transform.mutable_position()->set_z(position[2]);

                transform.mutable_rotation()->set_w(rotation[0]);
                transform.mutable_rotation()->set_x(rotation[1]);
                transform.mutable_rotation()->set_y(rotation[2]);
                transform.mutable_rotation()->set_z(rotation[3]);

                transform.mutable_scale()->set_x(scale[0]);
                transform.mutable_scale()->set_y(scale[1]);
                transform.mutable_scale()->set_z(scale[2]);
            }
        }

        return changed;
    }

    void PropertiesPanel::setSelectedEntityDef(std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> def)
    { 
        _selected_entity_def = std::move(def);
        if(_selected_entity_def)spdlog::debug("[editor] Selected entity def: {}", _selected_entity_def->second.name());
    }

    void PropertiesPanel::draw(const DrawContext& ctx) noexcept 
    {
        if (!_visible) return;

        // dock into the node
        if (ctx.properties_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.properties_dock_id, ImGuiCond_Always);

        // No close button => cannot be closed by the user
        ImGuiWindowFlags flags =    
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar;


        if (ImGui::Begin("Properties", &_visible, flags)) {
            ImGui::TextUnformatted("Inspector");
            ImGui::Separator();

            // // Placeholder inspector for a transform-like component.
            // static float position[3] = {0.f, 0.f, 0.f};
            // static float rotation[3] = {0.f, 0.f, 0.f};
            // static float scale[3]    = {1.f, 1.f, 1.f};

            // if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     ImGui::DragFloat3("Position", position, 0.1f);
            //     ImGui::DragFloat3("Rotation", rotation, 0.5f);
            //     ImGui::DragFloat3("Scale",    scale,    0.01f, 0.01f, 100.0f);
            // }

            // if (ImGui::CollapsingHeader("Rendering")) {
            //     static int  layer   = 0;
            //     static bool visible = true;
            //     ImGui::Checkbox("Visible", &visible);
            //     ImGui::InputInt("Layer", &layer);
            // }

            if(_selected_entity_def)
            {

                auto & [chunk_id, entity] = *_selected_entity_def;
                ImGui::TextUnformatted(entity.name().c_str());

                if(entity.has_transform())
                {
                    _properties_changed |= _drawTransformComponent(*(entity.mutable_transform()));
                }
            }
        }
        ImGui::End();
    }
}