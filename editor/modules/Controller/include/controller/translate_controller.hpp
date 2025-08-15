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
#include "world/world.hpp"   // whatever header defines WorldStreamer/ChunkID
// ^ names as in your tree (adjust include if needed)

namespace astre::editor::controller
{
    class TranslateController {
    public:
        enum class Axis : uint8_t { None=0, X=1, Y=2, Z=3 };

        struct Options {
            float screen_pick_px = 10.0f;   // hover distance in screen space (pixels)
            float gizmo_scale = 0.25f;      // scales with distance to camera
            float min_len = 0.5f;
            float max_len = 6.0f;
            float shaft_radius = 0.03f;     // relative to length
            float tip_len_ratio = 0.22f;
            float tip_radius_ratio = 0.08f;
        };

        TranslateController(
            render::IRenderer& renderer,
            ecs::Registry& registry,
            world::WorldStreamer& streamer) noexcept
        : _renderer(renderer)
        , _registry(registry)
        , _streamer(streamer)
        {}

        // Call once per frame from your “UI & overlays builder” stage.
        // Will append the gizmo’s RenderProxies to 'frame.render_proxies' (Debug phase).
        void update(
            render::Frame& frame,
            const math::Mat4& view,
            const math::Mat4& proj,
            const math::Vec2& viewport_pos_px,
            const math::Vec2& viewport_size_px,
            input::InputService& input,
            std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> selected,
            float dt) noexcept
        {
            _dt = dt;
            _vp_pos  = viewport_pos_px;
            _vp_size = viewport_size_px;

            // Nothing selected? Clear state and bail.
            if (!selected || !selected->second.has_transform()) {
                _hover = Axis::None;
                if (_active != Axis::None) _endDragCommit(*_active_def);
                _active = Axis::None;
                return;
            }
            _active_def = &selected->second;
            _active_chunk = selected->first;

            // Current target world position
            const math::Vec3 pos = math::deserialize(_active_def->transform().position());

            // Camera info
            const math::Mat4 VP = proj * view;
            const math::Mat4 invV  = glm::inverse(view);
            const math::Mat4 invVP = glm::inverse(VP);
            const math::Vec3 cam_pos = math::Vec3(invV[3]);

            // Distance-scaled size
            const float dist = glm::length(pos - cam_pos);
            const float len  = glm::clamp(dist * _opt.gizmo_scale, _opt.min_len, _opt.max_len);
            const float shaft_r = len * _opt.shaft_radius;
            const float tip_len = len * _opt.tip_len_ratio;
            const float tip_r   = len * _opt.tip_radius_ratio;

            // World axes from entity rotation (if present), else identity
            math::Quat rot = math::deserialize(_active_def->transform().rotation());
            math::Vec3 axX = glm::normalize(rot * math::Vec3(1,0,0));
            math::Vec3 axY = glm::normalize(rot * math::Vec3(0,1,0));
            math::Vec3 axZ = glm::normalize(rot * math::Vec3(0,0,1));

            // Hover test (only when not dragging)
            const auto mouse = input.getMousePosition(); // has x,y in window client space :contentReference[oaicite:0]{index=0}
            if (_active == Axis::None) {
                _hover = _hitTest(mouse, pos, {axX,axY,axZ}, len, VP) ;
            }

            // Begin / update / end drag
            const bool lmb_down     = input.isKeyHeld(input::InputCode::MOUSE_LEFT);        // :contentReference[oaicite:1]{index=1}
            const bool lmb_pressed  = input.isKeyJustPressed(input::InputCode::MOUSE_LEFT); // :contentReference[oaicite:2]{index=2}
            const bool lmb_released = input.isKeyJustReleased(input::InputCode::MOUSE_LEFT);// :contentReference[oaicite:3]{index=3}

            if (_active == Axis::None && lmb_pressed && _hover != Axis::None && _inViewport(mouse)) {
                _beginDrag(mouse, pos, _axisDir(_hover, axX, axY, axZ), invVP, cam_pos);
            } else if (_active != Axis::None && lmb_down) {
                _updateDrag(mouse, invVP, cam_pos);
            } else if (_active != Axis::None && lmb_released) {
                _endDragCommit(*_active_def);
                _active = Axis::None;
            }

            // Draw gizmo as overlay (cylinder + cone per axis) with Debug shader.
            _emitAxis(frame, pos, axX, len, shaft_r, tip_len, tip_r, Axis::X, math::Vec4(1,0,0,1));
            _emitAxis(frame, pos, axY, len, shaft_r, tip_len, tip_r, Axis::Y, math::Vec4(0,1,0,1));
            _emitAxis(frame, pos, axZ, len, shaft_r, tip_len, tip_r, Axis::Z, math::Vec4(0,0,1,1));
        }

        void setOptions(const Options& opt) noexcept { _opt = opt; }

    private:
        // ---------- math helpers ----------
        static math::Vec2 _toNDC(const math::Vec2& mouse_px, const math::Vec2& vp_pos, const math::Vec2& vp_size) noexcept {
            const float x = ((mouse_px.x - vp_pos.x) / glm::max(vp_size.x, 1.0f)) * 2.0f - 1.0f;
            const float y = 1.0f - ((mouse_px.y - vp_pos.y) / glm::max(vp_size.y, 1.0f)) * 2.0f; // flip Y
            return {x,y};
        }

        static void _buildRay(const math::Vec2& ndc, const math::Mat4& invVP, const math::Vec3& cam_pos,
                              math::Vec3& out_origin, math::Vec3& out_dir) noexcept
        {
            const math::Vec4 n_clip(ndc.x, ndc.y, -1.f, 1.f);
            const math::Vec4 f_clip(ndc.x, ndc.y,  1.f, 1.f);
            math::Vec4 n_world = invVP * n_clip; n_world /= n_world.w;
            math::Vec4 f_world = invVP * f_clip; f_world /= f_world.w;
            out_origin = cam_pos;
            out_dir = glm::normalize(math::Vec3(f_world - n_world));
        }

        static bool _closestPointsLineRay(
            const math::Vec3& L0, const math::Vec3& u,   // axis line: P = L0 + s*u
            const math::Vec3& R0, const math::Vec3& v,   // mouse ray: R = R0 + t*v
            float& s_out) noexcept
        {
            const math::Vec3 w0 = L0 - R0;
            const float a = glm::dot(u,u);
            const float b = glm::dot(u,v);
            const float c = glm::dot(v,v);
            const float d = glm::dot(u,w0);
            const float e = glm::dot(v,w0);
            const float denom = a*c - b*b;
            if (std::abs(denom) < 1e-6f) return false; // nearly parallel: skip
            s_out = (b*e - c*d) / denom;
            return true;
        }

        // ---------- hit testing in screen space ----------
        Axis _hitTest(const math::Vec2& mouse_px, const math::Vec3& origin,
                      const std::array<math::Vec3,3>& dirs, float len, const math::Mat4& VP) const noexcept
        {
            const auto segDist = [&](const math::Vec3& a, const math::Vec3& b)->float {
                auto toScreen = [&](const math::Vec3& p)->math::Vec2{
                    const math::Vec4 clip = VP * math::Vec4(p, 1.f);
                    if (clip.w <= 0.f) return math::Vec2(std::numeric_limits<float>::infinity());
                    const math::Vec3 ndc = math::Vec3(clip) / clip.w;
                    // back to px
                    math::Vec2 s;
                    s.x = _vp_pos.x + (ndc.x * 0.5f + 0.5f) * _vp_size.x;
                    s.y = _vp_pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * _vp_size.y;
                    return s;
                };
                const math::Vec2 p0 = toScreen(a);
                const math::Vec2 p1 = toScreen(b);
                if (!std::isfinite(p0.x) || !std::isfinite(p1.x)) return std::numeric_limits<float>::infinity();
                const math::Vec2 v = p1 - p0;
                const float l2 = glm::dot(v,v);
                if (l2 <= 1e-6f) return glm::length(mouse_px - p0);
                float t = glm::dot(mouse_px - p0, v) / l2;
                t = glm::clamp(t, 0.f, 1.f);
                const math::Vec2 proj = p0 + t*v;
                return glm::length(mouse_px - proj);
            };

            const float dx = segDist(origin, origin + dirs[0]*len);
            const float dy = segDist(origin, origin + dirs[1]*len);
            const float dz = segDist(origin, origin + dirs[2]*len);
            const float th = _opt.screen_pick_px;

            Axis best = Axis::None;
            float bestd = th + 1.0f;
            if (dx < bestd && dx <= th) { best = Axis::X; bestd = dx; }
            if (dy < bestd && dy <= th) { best = Axis::Y; bestd = dy; }
            if (dz < bestd && dz <= th) { best = Axis::Z; bestd = dz; }
            return best;
        }

        // ---------- drag state ----------
        void _beginDrag(const math::Vec2& mouse_px, const math::Vec3& start_pos,
                        const math::Vec3& dir, const math::Mat4& invVP, const math::Vec3& cam_pos) noexcept
        {
            _active = _hover;
            _start_pos = start_pos;
            _axis_dir = dir;

            const math::Vec2 ndc = _toNDC(mouse_px, _vp_pos, _vp_size);
            math::Vec3 ro, rd;
            _buildRay(ndc, invVP, cam_pos, ro, rd);
            _s0_valid = _closestPointsLineRay(_start_pos, _axis_dir, ro, rd, _s0);
        }

        void _updateDrag(const math::Vec2& mouse_px, const math::Mat4& invVP, const math::Vec3& cam_pos) noexcept
        {
            if (_active == Axis::None || !_s0_valid || !_active_def) return;

            const math::Vec2 ndc = _toNDC(mouse_px, _vp_pos, _vp_size);
            math::Vec3 ro, rd;
            _buildRay(ndc, invVP, cam_pos, ro, rd);
            float s_now{};
            if (!_closestPointsLineRay(_start_pos, _axis_dir, ro, rd, s_now)) return;

            const float delta_s = s_now - _s0;
            const math::Vec3 new_pos = _start_pos + (_axis_dir * delta_s);

            // Live update ECS (smooth visual) — mutate TransformComponent in the registry.
            // if (ecs::Entity e = _active_def->id(); e != ecs::INVALID_ENTITY) { // name -> entity id :contentReference[oaicite:4]{index=4}
            //     if (auto* tc = _registry.getComponent<ecs::TransformComponent>(e)) {                  // direct component access is available :contentReference[oaicite:5]{index=5}
            //         auto pos_ser = tc->mutable_position();
            //         pos_ser->set_x(new_pos.x);
            //         pos_ser->set_y(new_pos.y);
            //         pos_ser->set_z(new_pos.z);
            //     }
            // }

            // Update the pending definition so the inspector reflects it as well.
            auto* tdef = _active_def->mutable_transform();
            tdef->mutable_position()->set_x(new_pos.x);
            tdef->mutable_position()->set_y(new_pos.y);
            tdef->mutable_position()->set_z(new_pos.z);
        }

        void _endDragCommit(ecs::EntityDefinition& def) noexcept
        {
            if (!def.has_transform()) return;
            // Commit to archive/chunk only once (on mouse release).
            // WorldStreamer already exposes updateEntity(chunk, EntityDefinition) in your code. 
            _streamer.updateEntity(_active_chunk, def); // adjusts saved entity in chunk file
        }

        // ---------- rendering ----------
        void _emitAxis(render::Frame& frame, const math::Vec3& origin, const math::Vec3& dir,
                       float len, float shaft_r, float tip_len, float tip_r,
                       Axis axis, math::Vec4 color) noexcept
        {
            const bool is_hovered = (_hover == axis);
            const bool is_active  = (_active == axis);

            // Visual feedback
            if (is_active) color = math::Vec4(1,1,0,1);          // yellow while dragging
            else if (is_hovered) color = color * 0.6f + 0.4f;    // brighten on hover

            // Assume cylinder/cone prefabs exist in VB library (loaded at startup).
            // Names: "cylinder_prefab", "cone_prefab".
            const auto vb_cyl_res = _renderer.getVertexBuffer("cylinder_prefab");
            if(!vb_cyl_res)
            {
                spdlog::error("[controller] missing cylinder prefab");
                return;
            }
            const auto & vb_cyl = *vb_cyl_res;
            const auto vb_cone_res = _renderer.getVertexBuffer("cone_prefab");
            if(!vb_cone_res)
            {
                spdlog::error("[controller] missing cone prefab");
                return;
            }
            const auto & vb_cone = *vb_cone_res;

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
            auto proxy_shaft = render::RenderProxy{
                .visible = true,
                .phases = render::RenderPhase::Debug,
                .vertex_buffer = vb_cyl,
                .inputs{
                    .in_vec4 = {{"uColor", color}},
                    .in_mat4 = {{"uModel", M_shaft}}
                }
            };
            auto proxy_tip = render::RenderProxy{
                .visible = true,
                .phases = render::RenderPhase::Debug,
                .vertex_buffer = vb_cyl,
                .inputs{
                    .in_vec4 = {{"uColor", color}},
                    .in_mat4 = {{"uModel", M_tip}}
                }
            };

            // Unique, stable IDs so they overwrite each frame
            const std::size_t base = 0xC0FFEE00;
            const uint32_t a = static_cast<uint32_t>(axis);
            frame.render_proxies[base + (a*2+0)] = std::move(proxy_shaft);
            frame.render_proxies[base + (a*2+1)] = std::move(proxy_tip);
        }

        static math::Vec3 _axisDir(Axis a, const math::Vec3& X, const math::Vec3& Y, const math::Vec3& Z) noexcept {
            switch (a) { case Axis::X: return X; case Axis::Y: return Y; case Axis::Z: return Z; default: return math::Vec3(0); }
        }

        bool _inViewport(const math::Vec2& mouse_px) const noexcept {
            return mouse_px.x >= _vp_pos.x && mouse_px.y >= _vp_pos.y &&
                   mouse_px.x <= (_vp_pos.x + _vp_size.x) &&
                   mouse_px.y <= (_vp_pos.y + _vp_size.y);
        }

    private:
        render::IRenderer&  _renderer;
        ecs::Registry&      _registry;
        world::WorldStreamer& _streamer;

        Options _opt{};

        // State
        Axis _hover{Axis::None};
        Axis _active{Axis::None};
        float _dt{0.f};
        math::Vec2 _vp_pos{0};
        math::Vec2 _vp_size{1,1};

        math::Vec3 _start_pos{0};
        math::Vec3 _axis_dir{0,1,0};
        float _s0{0.f};
        bool  _s0_valid{false};

        ecs::EntityDefinition* _active_def{nullptr};
        world::ChunkID _active_chunk{};
    };
} // namespace astre::editor::controller
