#pragma once
#include <string_view>

#include "panel/panel_interface.hpp"

namespace astre::editor::panel 
{

    class ViewportPanel final : public IPanel 
    {
    public:
        ViewportPanel() = default;

        std::string_view name() const noexcept override { return "Viewport"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

        const math::Vec2& getImgPos() const noexcept { return _img_pos; }
        const math::Vec2& getImgSize() const noexcept { return _img_size; }

        bool isHovered() const noexcept { return _hovered; }

    private:
        bool _visible{true};
        bool _hovered{false};

        math::Vec2 _img_pos;
        math::Vec2 _img_size;
    };

}
