#pragma once
#include <string_view>

#include "panel/panel_interface.hpp"

namespace astre::editor::panel
{

    class PropertiesPanel final : public IPanel
    {
    public:
        PropertiesPanel() = default;

        std::string_view name() const noexcept override { return "Properties"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

    private:
        bool _visible{true};
    };

}