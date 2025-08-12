#include <imgui.h>

#include "panel/assets_panel.hpp"

namespace astre::editor::panel
{
    static bool _drawVertexBuffers(const absl::flat_hash_set<std::string> & loaded_vertex_buffers )
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (ImGui::TreeNodeEx("models/", flags))
        {
            for (const auto & vb : loaded_vertex_buffers)
            {
                ImGui::BulletText("%s", vb.c_str());
            }
            ImGui::TreePop();
        }

        return false;
    }

    static bool _drawShaders(const absl::flat_hash_set<std::string> & loaded_shaders )
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (ImGui::TreeNodeEx("shaders/", flags))
        {
            for (const auto & sh : loaded_shaders)
            {
                ImGui::BulletText("%s", sh.c_str());
            }
            ImGui::TreePop();
        }

        return false;
    }

    static bool _drawScripts(const absl::flat_hash_set<std::string> & loaded_scripts )
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (ImGui::TreeNodeEx("scripts/", flags))
        {
            for (const auto & sh : loaded_scripts)
            {
                ImGui::BulletText("%s", sh.c_str());
            }
            ImGui::TreePop();
        }

        return false;
    }


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
                _drawVertexBuffers(_tracker.getLoadedVertexBuffers());
                ImGui::BulletText("materials/");
                ImGui::BulletText("scenes/");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("resources")) {
                _drawShaders(_tracker.getLoadedShaders());
                _drawScripts(_tracker.getLoadedScripts());
                ImGui::BulletText("fonts/");
                ImGui::TreePop();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // content area

            ImGui::BeginChild("##content", ImVec2(0, 0), true);
            ImGui::TextDisabled("Folder: assets/models");
            ImGui::Separator();
            // // A simple list view; later replace with thumbnails + async scan
            // ImGui::Selectable("cube.fbx");
            // ImGui::Selectable("character.glb");
            // ImGui::Selectable("room.obj");

            ImGui::EndChild();

            (void)ctx;
        }
        ImGui::End();
    }
}