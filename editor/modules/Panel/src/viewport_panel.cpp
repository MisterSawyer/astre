#include <imgui.h>

#include "panel/viewport_panel.hpp"

namespace astre::editor::panel
{
    static void _drawCameraOverlay(const math::Vec3 & camera_position) noexcept
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (!dl) return;

        const ImVec2 win_pos  = ImGui::GetWindowPos();
        const ImVec2 cr_min   = ImGui::GetWindowContentRegionMin();
        const ImVec2 cr_max   = ImGui::GetWindowContentRegionMax();
        const ImVec2 avail_sz = ImVec2(cr_max.x - cr_min.x, cr_max.y - cr_min.y);
        if (avail_sz.x <= 2.0f || avail_sz.y <= 2.0f) return;

        // Text
        char line[128];
        std::snprintf(line, sizeof(line), "Cam: [%.2f, %.2f, %.2f]",
                      camera_position.x, camera_position.y, camera_position.z);

        const ImVec2 text_sz = ImGui::CalcTextSize(line);
        const float pad = 6.0f;
        const float margin = 8.0f; // distance from the content edges
        const float rounding = ImGui::GetStyle().FrameRounding;

        // Build rect from bottom-right of content region inward
        const ImVec2 rect_max = ImVec2(
            win_pos.x + cr_max.x - margin,
            win_pos.y + cr_max.y - margin
        );
        const ImVec2 rect_min = ImVec2(
            rect_max.x - (text_sz.x + 2.0f * pad),
            rect_max.y - (text_sz.y + 2.0f * pad)
        );

        const ImVec2 anchor = ImVec2(rect_min.x + pad, rect_min.y + pad); // text top-left

        // Background & border
        dl->AddRectFilled(rect_min, rect_max, IM_COL32(15, 15, 20, 200), rounding);
        dl->AddRect      (rect_min, rect_max, IM_COL32(255,255,255, 60), rounding);

        // Text
        dl->AddText(anchor, IM_COL32(255,255,255,255), line);
    }
    


    static void _drawStatsOverlayAt(const ImVec2& anchor,
                                float logic_fps, float logic_frame_time,
                                const astre::render::FrameStats& stats) noexcept
    {
        constexpr ImVec2 kWindowPivot = ImVec2(0.0f, 0.0f);

        // Transparent, click-through, auto-sized overlay window
        ImGui::SetNextWindowPos(anchor, ImGuiCond_Always, kWindowPivot);                // absolute screen pos
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 6.f));

        ImGui::Begin("StatsOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoInputs
        );

        const ImGuiIO& io = ImGui::GetIO();
        if (io.Framerate > 0.0f) {
            ImGui::Text("Render FPS: %.1f", io.Framerate);
            ImGui::Text("Render Frame Time: %.2f ms", 1000.0f / io.Framerate);
        }
        ImGui::Text("Logic FPS: %.1f", logic_fps);
        ImGui::Text("Logic Frame Time: %.2f ms", logic_frame_time);
        ImGui::Text("Draw Calls: %d", stats.draw_calls);
        ImGui::Text("Vertices: %d", stats.vertices);
        ImGui::Text("Triangles: %d", stats.triangles);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::draw(panel::DrawContext& ctx) noexcept
    {
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

            if (ctx.viewport_texture)
            {
                ImGui::Image(
                    (ImTextureID)(intptr_t)ctx.viewport_texture,
                    avail,
                    ImVec2(0.0f, 1.0f),  // uv0
                    ImVec2(1.0f, 0.0f)   // uv1 (flipped)
                );

                // query the actual image rectangle
                const ImVec2 img_min = ImGui::GetItemRectMin();
                const ImVec2 img_max = ImGui::GetItemRectMax();

                _img_size.x = img_max.x - img_min.x;
                _img_size.y = img_max.y - img_min.y;
                
                _img_pos.x = img_min.x;
                _img_pos.y = img_min.y;

                _hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            }

            // Compute anchor at top-left
            const ImVec2 pad = ImVec2(10.f, 30.f);
            const ImVec2 win_pos     = ImGui::GetWindowPos();              // in screen space
            const ImVec2 anchor      = ImVec2(win_pos.x + pad.x,
                                              win_pos.y + pad.y);

            // Submit overlay *after* the image so it appears on top
            _drawStatsOverlayAt(anchor, ctx.logic_fps, ctx.logic_frame_time, ctx.stats);

            _drawCameraOverlay(ctx.camera_position);
        }

        ImGui::End();

    }
}