#pragma once
#include <cmath>

#include <spdlog/spdlog.h>
#include <imgui.h>

#include "process/process.hpp"
#include "math/math.hpp"
#include "render/render.hpp"
#include "input/input.hpp"
#include "pipeline/pipeline.hpp"

namespace astre::editor::controller 
{

    // Editor-only fly camera
    class EditorFlyCamera 
    {
    public:
        // Tunables
        float baseSpeed     = 8.0f;     // m/s
        float mouseSens     = 0.5f;    // deg/pixel
        float vfovDegrees   = 60.0f;    // vertical FOV
        float zNear         = 0.05f;
        float zFar          = 1000.0f;

        EditorFlyCamera()
        {
            _calculateForward();
            _right = math::normalize(math::cross(_forward, math::Vec3(0,1,0)));
            _calculateViewMatrix(); 
        }

        asio::awaitable<void> update(float dt, process::IProcess & process, input::InputService & input) noexcept
        {
            co_await _handleInput(process, input);

            if (_captured == false || _vp_hovered == false)co_return;
            _updatePosition(dt);
        }

        // Apply to render::Frame (projection built with the given aspect)
        void overrideFrame(render::Frame& frame, float aspect) const noexcept
        {
            frame.view_matrix = _view_matrix;
            frame.proj_matrix = math::perspective(math::radians(vfovDegrees), std::max(0.01f, aspect), zNear, zFar);
            frame.camera_position = _position;
        }

        void setCaptured(bool captured) noexcept { _captured = captured; }
        [[nodiscard]] bool isCaptured() const noexcept { return _captured; }

        void setHovered(bool hovered) noexcept { _vp_hovered = hovered; }
        [[nodisard]] bool isHovered() const noexcept { return _vp_hovered; }

        // Viewport info (provided by ViewportPanel each frame)
        void setViewportRect(math::Vec2 content_top_left, math::Vec2 size) noexcept {
            _vp_pos.x = content_top_left.x;
            _vp_pos.y = content_top_left.y;
            _vp_size.x = size.x;
            _vp_size.y = size.y;
        }

        [[nodiscard]] math::Vec2 getViewportSize() const noexcept { return {_vp_size.x, _vp_size.y}; }
        [[nodiscard]] math::Vec2 getViewportPosition() const noexcept { return {_vp_pos.x, _vp_pos.y}; }

        const math::Vec3 & getPosition() const noexcept { return _position; }
        const math::Vec3 & getForward() const noexcept { return _forward; }
        const math::Vec3 & getRight() const noexcept { return _right; }

        void moveForward() noexcept {
            if(!_captured) 
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos += _forward;
        }

        void moveBackward() noexcept {
            if(!_captured)
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos -= _forward;
        }

        void moveRight() noexcept {
            if(!_captured)
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos += _right;
        }

        void moveLeft() noexcept {
            if(!_captured)
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos -= _right;
        }

        void moveUp() noexcept {
            if(!_captured)
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos += math::Vec3(0,1,0);
        }

        void moveDown() noexcept {
            if(!_captured)
            {
                _delta_pos = math::Vec3(0,0,0);
                return;
            } 
            _delta_pos -= math::Vec3(0,1,0);
        }

        void addDeltaYaw(float delta_yaw) noexcept { 
            if(!_captured)
            {
                _delta_yaw = 0.0f;
                return;
            }
            _delta_yaw += delta_yaw;
        }
        
        void addDeltaPitch(float delta_pitch) noexcept {
            if(!_captured)
            {
                _delta_pitch = 0.0f;
                return;
            }
            _delta_pitch += delta_pitch;
        }

    private:
        asio::awaitable<void> _handleInput(process::IProcess & process, const input::InputService & input)
        {
            if(isHovered() && input.isKeyJustReleased(input::InputCode::MOUSE_RIGHT))
            {
                bool captured = isCaptured();
                setCaptured(!captured);

                if(!captured)
                {
                    co_await process.hideCursor();
                }
                else
                {
                    co_await process.showCursor();
                }
            }

            auto md = input.getMouseDelta();
            addDeltaYaw(md.x);
            addDeltaPitch(-md.y);

            if(input.isKeyHeld(input::InputCode::KEY_W))
            {
                moveForward();
            }

            if(input.isKeyHeld(input::InputCode::KEY_S))
            {
                moveBackward();
            }

            if(input.isKeyHeld(input::InputCode::KEY_A))
            {
                moveLeft();
            }

            if(input.isKeyHeld(input::InputCode::KEY_D))
            {
                moveRight();
            }

            if(input.isKeyHeld(input::InputCode::KEY_E))
            {
                moveUp();
            }

            if(input.isKeyHeld(input::InputCode::KEY_Q))
            {
                moveDown();
            }
        }

        void _updatePosition(float dt)
        {
            _yaw_deg   += (_delta_yaw * mouseSens);
            _pitch_deg += (_delta_pitch * mouseSens);

            if(_pitch_deg > 89.9f) _pitch_deg = 89.9f;
            if(_pitch_deg < -89.9f) _pitch_deg = -89.9f;

            if (math::length2(_delta_pos) > 0.0f) {
                _position += math::normalize(_delta_pos) * (baseSpeed * dt);
            }

            _calculateForward();
            _right = math::normalize(math::cross(_forward, math::Vec3(0,1,0)));
            _calculateViewMatrix();

            _delta_pos = math::Vec3(0,0,0);
            _delta_yaw = 0.0f;
            _delta_pitch = 0.0f;
        }

        void _calculateForward() noexcept
        {
            const float yaw_R   = math::radians(_yaw_deg);
            const float pitch_R = math::radians(_pitch_deg);
            _forward =  math::normalize(math::Vec3(
                std::cos(pitch_R)*std::cos(yaw_R),
                std::sin(pitch_R),
                std::cos(pitch_R)*std::sin(yaw_R)
            ));
        }

        void _calculateViewMatrix() noexcept
        {
            _view_matrix = math::lookAt(_position, _position + _forward, {0,1,0});
        }

        ImVec2 _vp_pos{0,0};
        ImVec2 _vp_size{0,0};
        bool _vp_hovered{false};
        bool _captured{false};

        // State
        math::Mat4 _view_matrix;
        math::Vec3 _forward;
        math::Vec3 _right;

        math::Vec3 _position{0.0f, 5.0f, 15.0f};
        math::Vec3 _delta_pos{0.0f, 0.0f, 0.0f};

        float _yaw_deg   = -90.0f; // face -Z by default
        float _delta_yaw = 0.0f;

        float _pitch_deg =  -10.0f;
        float _delta_pitch = 0.0f;
    };

}