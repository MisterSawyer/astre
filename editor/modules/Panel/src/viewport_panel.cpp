#include <imgui.h>

#include "panel/viewport_panel.hpp"

namespace astre::editor::panel
{
    void ViewportPanel::draw(const panel::DrawContext& ctx) noexcept {
        if (!_visible) return;

        // dock into the node
        if (ctx.viewport_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.viewport_dock_id, ImGuiCond_Always);


        // No title/close/menu; resizable by splitters only
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin("Viewport", &_visible, flags))
        {
            ImGui::TextUnformatted("Viewport");


            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const int w = (int)avail.x;
            const int h = (int)avail.y;

            if (ctx.viewport_texture)
            {
                ImGui::Image(
                    (ImTextureID)(intptr_t)ctx.viewport_texture,
                    ImVec2((float)w, (float)h),
                    ImVec2(0.0f, 1.0f),  // uv0
                    ImVec2(1.0f, 0.0f)   // uv1 (flipped)
                );
            }

            ImGui::End();
        }
    }
}