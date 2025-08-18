// editor/modules/Controller/include/controller/translate_gizmo.hpp
#pragma once
#include <optional>
#include <array>
#include <cstdint>
#include <limits>

#include "math/math.hpp"
#include "render/render.hpp"
#include "ecs/registry.hpp"
#include "generated/ECS/proto/entity_definition.pb.h"
#include "generated/ECS/proto/components/transform_component.pb.h"
#include "input/input.hpp"

#include "model/panel_draw_context.hpp"

namespace astre::editor::controller
{
    class TranslateOverlayController {
    public:
        struct Options {
            float screen_pick_px = 10.0f;   // hover distance in screen space (pixels)
            float gizmo_scale = 0.15f;      // scales with distance to camera
            float min_len = 0.5f;
            float max_len = 6.0f;
            float shaft_radius = 0.03f;     // relative to length
            float tip_len_ratio = 0.22f;
            float tip_radius_ratio = 0.08f;
        };

        TranslateOverlayController(render::IRenderer& renderer) noexcept
        {
            const auto vb_cyl_res = renderer.getVertexBuffer("cylinder_prefab");
            if(!vb_cyl_res)
            {
                spdlog::error("[controller] missing cylinder prefab");
                return;
            }
            _vb_cyl = *vb_cyl_res;

            const auto vb_cone_res = renderer.getVertexBuffer("cone_prefab");
            if(!vb_cone_res)
            {
                spdlog::error("[controller] missing cone prefab");
                return;
            }
            _vb_cone = *vb_cone_res;

            for(auto & shaft : _shafts)
            {
                shaft = render::RenderProxy{
                    .visible = true,
                    .phases = render::RenderPhase::Debug,
                    .vertex_buffer = _vb_cyl
                };
                shaft.options = render::RenderOptions
                {
                    .mode = render::RenderMode::Solid,
                    .write_depth = false,
                };

            }

            for(auto & tip : _tips)
            {
                tip = render::RenderProxy{
                    .visible = true,
                    .phases = render::RenderPhase::Debug,
                    .vertex_buffer = _vb_cone
                };
                tip.options = render::RenderOptions
                {
                    .mode = render::RenderMode::Solid,
                    .write_depth = false,
                };

            }
        }

        // Viewport info (provided by ViewportPanel each frame)
        void setViewportRect(math::Vec2 content_top_left, math::Vec2 size) noexcept {
            _vp_pos.x = content_top_left.x;
            _vp_pos.y = content_top_left.y;
            _vp_size.x = size.x;
            _vp_size.y = size.y;
        }

        void update(
            const model::DrawContext & ctx,
            input::InputService & input,
            const render::Frame & render_frame) noexcept
        {
            if(!ctx.selection_controller.isAnyEntitySelected()) 
            {
                _visible = false;
                _drag.dragging   = false;
                return;
            }
            const auto & selected_entity_def = ctx.selection_controller.getEntitySelection();

            if(selected_entity_def.has_transform() == false) 
            {
                _visible = false;
                _drag.dragging   = false;
                return;
            }

            // Current target world position
            const math::Vec3 pos = math::deserialize(selected_entity_def.transform().position());

            auto distance = math::length(pos - ctx.camera_position);

            const float len  = glm::clamp(distance * _opt.gizmo_scale, _opt.min_len, _opt.max_len);
            const float shaft_r = len * _opt.shaft_radius;
            const float tip_len = len * _opt.tip_len_ratio;
            const float tip_r   = len * _opt.tip_radius_ratio;

            // Axis colors (may be brightened on hover/drag)
            std::array<math::Vec4, 3> axis_colors = {
                math::Vec4(1,0,0,1),
                math::Vec4(0,1,0,1),
                math::Vec4(0,0,1,1)
            };

            const std::array<math::Vec3, 3> axis_dirs = {
                math::Vec3(1,0,0),
                math::Vec3(0,1,0),
                math::Vec3(0,0,1)
            };


            // ---- Picking / drag state machine ---------------------------------
            const bool can_interact = _vp_hovered;
            const bool lmb_down     = input.isKeyHeld(input::InputCode::MOUSE_LEFT);
            const bool lmb_pressed  = input.isKeyJustPressed(input::InputCode::MOUSE_LEFT);
            const bool lmb_released = input.isKeyJustReleased(input::InputCode::MOUSE_LEFT);
            const math::Vec2 mouse  = input.getMousePosition(); // window-space
            const math::Vec2 m_in_vp{ mouse.x - _vp_pos.x, mouse.y - _vp_pos.y };


            // Compute inverse matrices once
            const math::Mat4 invV = glm::inverse(render_frame.view_matrix);
            const math::Mat4 invP = glm::inverse(render_frame.proj_matrix);


            // Returns world ray dir from mouse (normalized)
            auto ray_dir_from_mouse = [&](math::Vec2 p/*in-vp*/) noexcept -> math::Vec3 {
                if (_vp_size.x <= 1e-6f || _vp_size.y <= 1e-6f) return math::normalize(ctx.camera_position - pos);
                // NDC
                const float x = (2.0f * (p.x / _vp_size.x)) - 1.0f;
                const float y = 1.0f - (2.0f * (p.y / _vp_size.y));
                math::Vec4 ray_clip{x, y, -1.0f, 1.0f};
                math::Vec4 ray_eye = invP * ray_clip;
                ray_eye = math::Vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
                math::Vec4 ray_world4 = invV * ray_eye;
                math::Vec3 rd = math::Vec3(ray_world4.x, ray_world4.y, ray_world4.z);
                return math::normalize(rd);
            };


            // Helper: project world to vp pixel space (top-left origin)
            auto world_to_vp = [&](const math::Vec3& w) noexcept -> math::Vec2 {
                math::Vec4 clip = render_frame.proj_matrix * render_frame.view_matrix * math::Vec4(w, 1.0f);
                const float invw = (std::abs(clip.w) > 1e-6f) ? (1.0f / clip.w) : 0.0f;
                const float ndc_x = clip.x * invw;
                const float ndc_y = clip.y * invw;
                const float sx = (ndc_x * 0.5f + 0.5f) * _vp_size.x;
                const float sy = (1.0f - (ndc_y * 0.5f + 0.5f)) * _vp_size.y;
                return math::Vec2(sx, sy);
            };

            // Screen-space segment distance: returns (distance_px, t in [0,1])
            auto seg_dist_px = [](const math::Vec2& p, const math::Vec2& a, const math::Vec2& b) noexcept -> std::pair<float,float> {
                const math::Vec2 ab = b - a;
                const float ab2 = glm::dot(ab, ab);
                float t = (ab2 > 1e-6f) ? glm::dot(p - a, ab) / ab2 : 0.0f;
                t = glm::clamp(t, 0.0f, 1.0f);
                const math::Vec2 q = a + ab * t;
                const float d = glm::length(p - q);
                return {d, t};
            };

            // Hover test (if not dragging)
            int hovered_axis = -1;
            if (!_drag.dragging && can_interact) {
                const math::Vec2 pm = m_in_vp;
                float best_d = std::numeric_limits<float>::max();
                for (int a = 0; a < 3; ++a) {
                    const math::Vec3 A0 = pos;
                    const math::Vec3 A1 = pos + axis_dirs.at(a) * len;
                    const math::Vec2 s0 = world_to_vp(A0);
                    const math::Vec2 s1 = world_to_vp(A1);
                    const auto [dpx, t] = seg_dist_px(pm, s0, s1);
                    (void)t;
                    if (dpx < best_d) { best_d = dpx; hovered_axis = a; }
                }
                if (hovered_axis >= 0 && best_d <= _opt.screen_pick_px) {
                    // brighten hovered
                    axis_colors[hovered_axis] = axis_colors[hovered_axis] * 0.6f + math::Vec4(0.4f);
                    if (lmb_pressed) {
                        // Begin drag: compute initial s on axis by closest points between ray and axis line
                        _drag.dragging   = true;
                        _drag.axis       = static_cast<unsigned>(hovered_axis);
                        _drag.origin     = pos;
                        _drag.start_pos  = pos;
                        _drag.mouse_start= pm;

                        const math::Vec3 d1 = ray_dir_from_mouse(pm);
                        const math::Vec3 d2 = glm::normalize(axis_dirs.at(_drag.axis));
                        const math::Vec3 r  = ctx.camera_position - pos;
                        const float a = 1.0f;
                        const float b = glm::dot(d1, d2);
                        const float c = glm::dot(d1, r);
                        const float e = 1.0f;
                        const float f = glm::dot(d2, r);
                        const float denom = (a*e - b*b);
                        _drag.s0 = (std::abs(denom) > 1e-6f) ? ((a*f - b*c) / denom) : 0.0f; // axis param at start
                    }
                }
            }

            // Update drag
            if (_drag.dragging) {
                axis_colors[_drag.axis] = math::Vec4(1,1,0,1); // active = yellow

                const math::Vec2 pm = m_in_vp;
                const math::Vec3 d1 = ray_dir_from_mouse(pm);
                const math::Vec3 d2 = glm::normalize(axis_dirs[_drag.axis]);
                const math::Vec3 r  = ctx.camera_position - _drag.origin;
                const float b = glm::dot(d1, d2);
                const float c = glm::dot(d1, r);
                const float f = glm::dot(d2, r);
                const float denom = (1.0f - b*b);

                float s_now = _drag.s0;
                if (std::abs(denom) > 1e-6f) {
                    s_now = (f - b*c) / denom;  // axis param now
                } else {
                    // Ray ~ parallel to axis -> fallback: use screen-projected scalar along axis direction
                    const math::Vec2 s0 = world_to_vp(_drag.origin);
                    const math::Vec2 s1 = world_to_vp(_drag.origin + d2);
                    const math::Vec2 ax = glm::normalize(s1 - s0);
                    const float delta_pixels = glm::dot(pm - _drag.mouse_start, ax);
                    // Roughly map pixels to world by using gizmo 'len' pixels span
                    const math::Vec2 sEnd = world_to_vp(_drag.origin + d2 * len);
                    const float axis_pixels = glm::length(sEnd - s0);
                    const float px_to_world = (axis_pixels > 1e-3f) ? (len / axis_pixels) : 0.0f;
                    s_now = _drag.s0 + delta_pixels * px_to_world;
                }

                const math::Vec3 new_pos = _drag.start_pos + d2 * (s_now - _drag.s0);
                _last_dragged_pos = new_pos;

                if (lmb_released || !lmb_down) {
                    _drag.dragging = false;
                }
            }


            // Draw gizmo as overlay (cylinder + cone per axis) with Debug shader.
            _updateAxis(pos, axis_dirs.at(0), len, shaft_r, tip_len, tip_r, 0, axis_colors.at(0), render_frame);
            _updateAxis(pos, axis_dirs.at(1), len, shaft_r, tip_len, tip_r, 1, axis_colors.at(1), render_frame);
            _updateAxis(pos, axis_dirs.at(2), len, shaft_r, tip_len, tip_r, 2, axis_colors.at(2), render_frame);

            _visible = true;
        }

        void setOptions(const Options& opt) noexcept { _opt = opt; }


        void setHovered(bool hovered) noexcept { _vp_hovered = hovered; }
        [[nodisard]] bool isHovered() const noexcept { return _vp_hovered; }

        void draw(render::Frame & render_frame)
        {
            if(_visible == false) return;

            const std::size_t base = 0xC0FFEE00;

            // TODO Unique, stable IDs so they overwrite each frame
            for(unsigned short axis = 0; axis < 3; ++axis)
            {
                render_frame.render_proxies[base + (axis*2 + 0)] = _shafts.at(axis);
                render_frame.render_proxies[base + (axis*2 + 1)] = _tips.at(axis);
            }
        }

        // If a drag is active this frame, returns the latest gizmo position; caller should persist into entity/streamer.
        std::optional<math::Vec3> takeDraggedPosition() noexcept {
            auto out = _last_dragged_pos;
            _last_dragged_pos.reset();
            return out;
        }

        bool isDragging() const noexcept { return _drag.dragging; }

    private:
        // ---------- rendering ----------
        void _updateAxis(const math::Vec3& origin, const math::Vec3& dir,
                       float len, float shaft_r, float tip_len, float tip_r,
                       unsigned short axis, math::Vec4 color, 
                       const render::Frame & render_frame) noexcept
        {
            const bool is_hovered = false;
            const bool is_active  = true;

            // Visual feedback
            //if (is_active) color = math::Vec4(1,1,0,1);          // yellow while dragging
            //else if (is_hovered) color = color * 0.6f + 0.4f;    // brighten on hover

            // Build transform to align +Y model to 'dir'
            const math::Quat q = glm::rotation(math::Vec3(0,1,0), glm::normalize(dir));
            const math::Mat4 R = glm::toMat4(q);

            // Shaft (centered at half length along axis)
            const math::Mat4 M_shaft =
                glm::translate(math::Mat4(1.f), origin + dir * (len * 0.5f)) *
                R *
                glm::scale(math::Mat4(1.f), math::Vec3(shaft_r, len * 0.5f, shaft_r)); // h=1 mapped to length

            // Tip (placed at end)
            const math::Mat4 M_tip =
                glm::translate(math::Mat4(1.f), origin + dir * (len + tip_len * 0.5f)) *
                R *
                glm::scale(math::Mat4(1.f), math::Vec3(tip_r, tip_len * 0.5f, tip_r));

            // Emit 2 proxies (shaft + tip). Debug stage ignores per-proxy shader and uses Debug Overlay shader. :contentReference[oaicite:6]{index=6}
            _shafts.at(axis).position = origin + dir * (len * 0.5f);
            _shafts.at(axis).rotation = q;
            _shafts.at(axis).scale = math::Vec3(shaft_r, len * 0.5f, shaft_r);
            _shafts.at(axis).inputs.in_vec4 = {{"uColor", color}};
            _shafts.at(axis).inputs.in_mat4 = {{"uModel", M_shaft}, {"uView", render_frame.view_matrix}, {"uProjection", render_frame.proj_matrix}};

            _tips.at(axis).position = origin + dir * (len + tip_len * 0.5f); 
            _tips.at(axis).rotation = q;
            _tips.at(axis).scale = math::Vec3(tip_r, tip_len * 0.5f, tip_r);
            _tips.at(axis).inputs.in_vec4 = {{"uColor", color}};
            _tips.at(axis).inputs.in_mat4 = {{"uModel", M_tip}, {"uView", render_frame.view_matrix}, {"uProjection", render_frame.proj_matrix}};
        }

    private:
        bool _visible = false;

        std::size_t _vb_cyl;
        std::size_t _vb_cone;

        Options _opt{};

        ImVec2 _vp_pos{0,0};
        ImVec2 _vp_size{0,0};
        bool _vp_hovered{false};
        bool _captured{false};

        std::array<render::RenderProxy, 3> _shafts;
        std::array<render::RenderProxy, 3> _tips;


        struct Drag {
            bool        dragging = false;
            unsigned    axis = 0;
            float       s0 = 0.0f;          // axis param at drag start
            math::Vec3  origin{};           // gizmo origin at drag start
            math::Vec3  start_pos{};        // entity pos at drag start
            math::Vec2  mouse_start{};      // mouse in-vp px at drag start
        } _drag;

        std::optional<math::Vec3> _last_dragged_pos{};
    };
} // namespace astre::editor::controller
