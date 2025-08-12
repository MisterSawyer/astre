#pragma once
#include <string_view>

#include "controller/editor_fly_camera.hpp"

#include "panel/panel_interface.hpp"

namespace astre::editor::panel 
{

    class ViewportPanel final : public IPanel 
    {
    public:
        ViewportPanel(controller::EditorFlyCamera & fly_camera) 
        : _fly_camera(fly_camera) {}

        std::string_view name() const noexcept override { return "Viewport"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

    private:
        bool _visible{true};

        controller::EditorFlyCamera & _fly_camera;
    };

}
