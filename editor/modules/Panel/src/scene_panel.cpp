#include <imgui.h>

#include "panel/scene_panel.hpp"

namespace astre::editor::panel
{
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
        
        if (ImGui::Begin("Scene", &_visible, flags)) {
            ImGui::TextUnformatted("Scene Hierarchy");
            ImGui::Separator();

            // Placeholder tree. Hook this up to EngineBridge later.
            if (ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BulletText("Camera");
                if (ImGui::TreeNode("Player")) {
                    ImGui::BulletText("MeshRenderer");
                    ImGui::BulletText("Script: PlayerController");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Light")) {
                    ImGui::BulletText("Directional");
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }

            ImGui::Spacing();
            ImGui::TextDisabled("(Connect to ECS to list real entities)");
            (void)ctx; // ctx.services.* available for selection signals later
        }
        ImGui::End();
    }
}