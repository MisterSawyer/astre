#pragma once
#include <string_view>

#include "panel/panel_interface.hpp"

namespace astre::editor::panel 
{
    class MainMenuBar final : public IPanel
    {
    public:
        MainMenuBar() = default;

        std::string_view name() const noexcept override { return "MainMenuBar"; }

        // Menu bar is conceptually always visible. Allow hiding if you want.
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(DrawContext& ctx) noexcept override;

    private:
        bool _visible{true};
    };

}
