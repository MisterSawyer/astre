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

        void draw(const model::DrawContext& ctx) noexcept override;

    private:
        enum class View : uint8_t { None, Models, Shaders, Scripts };


        bool _visible{true};

        asset::ResourceTracker & _tracker;

        // Content-area state
        View _current_view{View::Models};
        std::string _selected_item{};
        float _tile_size{64.0f};      // px; user adjustable in header bar
        float _tile_padding{6.0f};    // px around the tile

    };
}
