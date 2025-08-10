#include <imgui.h>

#include "panel/assets_panel.hpp"

namespace astre::editor::panel
{
    void AssetsPanel::draw(const DrawContext& ctx) noexcept
    {
        if (!_visible) return;

        // dock into the node
        if (ctx.assets_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.assets_dock_id, ImGuiCond_Always);

        // No close button => cannot be closed by the user
        ImGuiWindowFlags flags =    
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("Assets", &_visible, flags)) {
            ImGui::TextUnformatted("Asset Browser");
            ImGui::Separator();

            // Placeholder split: left folder tree, right grid/list
            ImGui::BeginChild("##folders", ImVec2(220, 0), true);
            if (ImGui::TreeNodeEx("assets", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BulletText("textures/");
                ImGui::BulletText("models/");
                ImGui::BulletText("materials/");
                ImGui::BulletText("scenes/");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("resources")) {
                ImGui::BulletText("shaders/");
                ImGui::BulletText("fonts/");
                ImGui::TreePop();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##content", ImVec2(0, 0), true);
            ImGui::TextDisabled("Folder: assets/models");
            ImGui::Separator();
            // A simple list view; later replace with thumbnails + async scan
            ImGui::Selectable("cube.fbx");
            ImGui::Selectable("character.glb");
            ImGui::Selectable("room.obj");
            ImGui::Spacing();
            ImGui::TextDisabled("(Connect Persistence/Async to scan disk & show thumbnails)");
            ImGui::EndChild();

            (void)ctx;
        }
        ImGui::End();
    }
}