#pragma once
#include <cmath>

#include <spdlog/spdlog.h>
#include <imgui.h>

#include "math/math.hpp"
#include "render/render.hpp"

namespace astre::editor::controller 
{

    // Editor-only fly camera controlled via ImGui input inside the viewport.
    class EditorFlyCamera {
    public:
        // Tunables
        float baseSpeed     = 8.0f;     // m/s
        float mouseSens     = 1.0f;    // deg/pixel
        float speedBoost    = 4.0f;     // Shift multiplier
        float speedSlow     = 0.25f;    // Ctrl multiplier
        float vfovDegrees   = 60.0f;    // vertical FOV
        float zNear         = 0.1f;
        float zFar          = 1000.0f;

        // State
        math::Vec3 position{0.0f, 5.0f, 15.0f};
        float yawDeg   = -90.0f; // face -Z by default
        float pitchDeg =  -10.0f;

        // Viewport info (provided by ViewportPanel each frame)
        void setViewportRect(ImVec2 content_top_left, ImVec2 size, bool hovered) noexcept {
            _vpPos = content_top_left; _vpSize = size; _vpHovered = hovered;
        }

        // Per-frame update; uses ImGui::GetIO for mouse/keyboard state.
        void update(float dtSeconds) noexcept
        {
            // control when holding RMB in the viewport
            _captured = _vpHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right);

            if (_captured == false)return; 
            
            ImGuiIO& io = ImGui::GetIO();

            ImGui::SetMouseCursor(ImGuiMouseCursor_None);

            // Use current frame MouseDelta for rotation
            const ImVec2 d = io.MouseDelta;
            yawDeg   += (d.x * mouseSens);
            pitchDeg -= (d.y * mouseSens);
            if (pitchDeg > 89.9f)  pitchDeg = 89.9f;
            if (pitchDeg < -89.9f) pitchDeg = -89.9f;

            //  Bind cursor to viewport center (backend-friendly)
            if (_vpSize.x > 2.0f && _vpSize.y > 2.0f) {
                const ImVec2 center{
                    _vpPos.x + _vpSize.x * 0.5f,
                    _vpPos.y + _vpSize.y * 0.5f
                };
                // Ask backend to set OS cursor here; ImGui will not count this as user delta.
                io.WantSetMousePos = true;
                io.MousePos = center;
            }

            // Scroll to adjust base speed when hovering viewport (nice UX while editing)
            if (_vpHovered && io.MouseWheel != 0.0f) {
                const float factor = (io.MouseWheel > 0.0f) ? 1.1f : (1.0f/1.1f);
                baseSpeed = std::clamp(baseSpeed * factor, 0.5f, 500.0f);
            }

            // WASD/E/Q movement (relative to yaw/pitch)
            const math::Vec3 fwd = forward();
            const math::Vec3 right = math::normalize(math::cross(fwd, math::Vec3(0,1,0)));
            const math::Vec3 up(0,1,0);

            float speed = baseSpeed;
            if (io.KeyShift) speed *= speedBoost;
            if (io.KeyCtrl)  speed *= speedSlow;

            math::Vec3 v(0);
            if (ImGui::IsKeyDown(ImGuiKey_W)) v += fwd;
            if (ImGui::IsKeyDown(ImGuiKey_S)) v -= fwd;
            if (ImGui::IsKeyDown(ImGuiKey_A)) v -= right;
            if (ImGui::IsKeyDown(ImGuiKey_D)) v += right;
            if (ImGui::IsKeyDown(ImGuiKey_E)) v += up;
            if (ImGui::IsKeyDown(ImGuiKey_Q)) v -= up;

            if (math::length2(v) > 0.0f) {
                position += math::normalize(v) * (speed * dtSeconds);
            }
        }

        // Apply to render::Frame (projection built with the given aspect)
        void overrideFrame(render::Frame& frame, float aspect) const noexcept {
            frame.view_matrix = viewMatrix();
            frame.proj_matrix = math::perspective(math::radians(vfovDegrees), std::max(0.01f, aspect), zNear, zFar);
            frame.camera_position = position;
        }

        // Accessors
        [[nodiscard]] math::Mat4 viewMatrix() const noexcept {
            return math::lookAt(position, position + forward(), {0,1,0});
        }
        [[nodiscard]] math::Vec3 forward() const noexcept {
            const float yawR   = math::radians(yawDeg);
            const float pitchR = math::radians(pitchDeg);
            return math::normalize(math::Vec3(
                std::cos(pitchR)*std::cos(yawR),
                std::sin(pitchR),
                std::cos(pitchR)*std::sin(yawR)
            ));
        }
        [[nodiscard]] bool captured() const noexcept { return _captured; }
        [[nodiscard]] ImVec2 viewportSize() const noexcept { return _vpSize; }

    private:
        ImVec2 _vpPos{0,0};
        ImVec2 _vpSize{0,0};
        bool _vpHovered{false};
        bool _captured{false};
    };

}