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

        void draw(const DrawContext& ctx) noexcept override;

        void setSelectedEntityDef(std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> def);

        bool propertiesChanged() const noexcept { return _properties_changed; }

        void resetPropertiesChanged() noexcept { _properties_changed = false; }

        std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> getSelectedEntityDef() const noexcept { return _selected_entity_def; }

    private:
        bool _visible{true};

        bool _properties_changed = false;

        std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> _selected_entity_def;
    };

}