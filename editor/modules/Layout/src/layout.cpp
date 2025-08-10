#include <imgui.h>
#include <imgui_internal.h>

#include "layout/layout.hpp"

namespace astre::editor::layout
{
    void DockSpace::build()
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();

        // Root full-viewport window + dockspace
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags root_flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("##AstreEditorRoot", nullptr, root_flags);
        ImGui::PopStyleVar(2);

        _dockspace_id = ImGui::GetID("AstreEditorDockSpace");

        // We want a passthrough central node so the scene can render "behind" it if desired.
        const ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_AutoHideTabBar;

        ImGui::DockSpace(_dockspace_id, ImVec2(0,0), dockspace_flags);

        // Build the layout once (or when we detect it missing).
        static bool layout_built = false;
        if (!layout_built ||ImGui::DockBuilderGetNode(_dockspace_id) == nullptr)
        {

            layout_built = true;

            ImGui::DockBuilderRemoveNode(_dockspace_id); // clear if any
            ImGui::DockBuilderAddNode(_dockspace_id, ImGuiDockNodeFlags_DockSpace | dockspace_flags);
            ImGui::DockBuilderSetNodeSize(_dockspace_id, vp->WorkSize);

            ImGuiID id_root      = _dockspace_id;
            ImGuiID id_left      = 0;
            ImGuiID id_right     = 0;
            ImGuiID id_center    = 0;
            ImGuiID id_center_top= 0;
            ImGuiID id_bottom    = 0;
            
            // 1) Left split: 20% width
            ImGui::DockBuilderSplitNode(id_root, ImGuiDir_Left, 0.20f,
                &id_left, &id_center);

            // 2) Right split: 20% width
            ImGui::DockBuilderSplitNode(id_center, ImGuiDir_Right,
                0.20f, &id_right, &id_center);

            // 3) Bottom split (from center): 30% height
            ImGui::DockBuilderSplitNode(id_center, ImGuiDir_Down, 0.30f,
                &id_bottom, &id_center_top);

            _left_dock_id    = id_left;
            _right_dock_id   = id_right;
            _bottom_dock_id  = id_bottom;
            _center_dock_id  = id_center_top;

            // hide their tab bars entirely so no "expand tab bar" nub appears.
            auto hide_tabbar = [](ImGuiID node_id) {
                if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(node_id)) {
                    n->LocalFlags |= ImGuiDockNodeFlags_NoTabBar; // requires imgui_internal.h
                }
            };
            hide_tabbar(_left_dock_id);
            hide_tabbar(_right_dock_id);
            hide_tabbar(_bottom_dock_id);
            hide_tabbar(_center_dock_id);

            ImGui::DockBuilderFinish(_dockspace_id);
        }

        ImGui::End(); // ##AstreEditorRoot
    }
}