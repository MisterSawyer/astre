#include <imgui.h>

#include "panel/properties_panel.hpp"

namespace astre::editor::panel 
{
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

            // Placeholder inspector for a transform-like component.
            static float position[3] = {0.f, 0.f, 0.f};
            static float rotation[3] = {0.f, 0.f, 0.f};
            static float scale[3]    = {1.f, 1.f, 1.f};

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", position, 0.1f);
                ImGui::DragFloat3("Rotation", rotation, 0.5f);
                ImGui::DragFloat3("Scale",    scale,    0.01f, 0.01f, 100.0f);
            }

            if (ImGui::CollapsingHeader("Rendering")) {
                static int  layer   = 0;
                static bool visible = true;
                ImGui::Checkbox("Visible", &visible);
                ImGui::InputInt("Layer", &layer);
            }

            ImGui::Spacing();
            ImGui::TextDisabled("(Hook to SelectionModel + reflection to show real data)");

            (void)ctx;
        }
        ImGui::End();
    }
}