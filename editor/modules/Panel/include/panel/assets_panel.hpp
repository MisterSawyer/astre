#pragma once
#include <string_view>

#include "asset/asset.hpp"

#include "panel/panel_interface.hpp"

namespace astre::editor::panel
{

    class AssetsPanel final : public IPanel
    {
    public:
        AssetsPanel(asset::ResourceTracker & tracker) noexcept 
        : _tracker(tracker) {};

        std::string_view name() const noexcept override { return "Assets"; }
        bool visible() const noexcept override { return _visible; }
        void setVisible(bool v) noexcept override { _visible = v; }

        void draw(const DrawContext& ctx) noexcept override;

    private:
        bool _visible{true};

        asset::ResourceTracker & _tracker;
    };
}
