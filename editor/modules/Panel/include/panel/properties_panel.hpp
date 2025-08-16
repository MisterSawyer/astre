#pragma once
#include <string_view>

#include "world/world.hpp"
#include "ecs/ecs.hpp"

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

        void draw(const model::DrawContext& ctx) noexcept override;

        bool updateSelectedEntity(controller::SelectionController & selection_controller);

    private:
        bool _visible{true};

        std::optional<ecs::EntityDefinition> pending_entity_def;
        bool properites_updated = false;
    };

}