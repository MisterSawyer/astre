#include <spdlog/spdlog.h>

#include "asset/entity_serializer.hpp"

namespace astre::asset
{
    template<class ComponentType>
    static ComponentSerializer _constructComponentSerializer(std::function<ComponentType *(ecs::EntityDefinition *)> mutable_component)
    {
        return 
            [mutable_component]
            (const ecs::Entity entity, const ecs::Registry & registry, ecs::EntityDefinition & entity_def) -> asio::awaitable<void>
            {
                if (!(co_await registry.hasComponent<ComponentType>(entity))) co_return;

                auto* target = mutable_component(&entity_def);
                if (!target)
                {
                    spdlog::error("Entity {} has null mutable component", entity);
                    throw std::runtime_error("mutable_component returned nullptr");
                }

                co_await registry.runOnSingleWithComponents<ComponentType>(entity, 
                [&target](const ecs::Entity entity, const ComponentType & component) -> asio::awaitable<void>
                {
                    target->CopyFrom(component);
                    co_return;
                });

                co_return; 
            };
    }

    EntitySerializer::EntitySerializer(const ecs::Registry & registry)
    : _registry(registry)
    {
        registerComponentSerializer("TransformComponent",
            _constructComponentSerializer<ecs::TransformComponent>(&ecs::EntityDefinition::mutable_transform) 
        );

        registerComponentSerializer("VisualComponent", 
            _constructComponentSerializer<ecs::VisualComponent>(&ecs::EntityDefinition::mutable_visual)
        );

        registerComponentSerializer("InputComponent",
            _constructComponentSerializer<ecs::InputComponent>(&ecs::EntityDefinition::mutable_input)
        );

        registerComponentSerializer("HealthComponent",
            _constructComponentSerializer<ecs::HealthComponent>(&ecs::EntityDefinition::mutable_health)
        );

        registerComponentSerializer("CameraComponent", 
            _constructComponentSerializer<ecs::CameraComponent>(&ecs::EntityDefinition::mutable_camera)
        );

        registerComponentSerializer("TerrainComponent", 
            _constructComponentSerializer<ecs::TerrainComponent>(&ecs::EntityDefinition::mutable_terrain)
        );

        registerComponentSerializer("LightComponent", 
            _constructComponentSerializer<ecs::LightComponent>(&ecs::EntityDefinition::mutable_light)
        );

        registerComponentSerializer("ScriptComponent", 
            _constructComponentSerializer<ecs::ScriptComponent>(&ecs::EntityDefinition::mutable_script)
        );
    }

    void EntitySerializer::registerComponentSerializer(const std::string& name, ComponentSerializer serializer)
    {
        _serializers[name] = std::move(serializer);
    }

    asio::awaitable<ecs::EntityDefinition> EntitySerializer::serializeEntity(const ecs::Entity entity) const {
        ecs::EntityDefinition entity_def;
        auto name_res = co_await _registry.getName(entity);
        if (name_res.has_value() == false) {
            spdlog::error("Entity {} has no name", entity);
            throw std::runtime_error("Entity has no name");
        }
        entity_def.set_name(name_res.value());
        for (const auto& [name, serializer] : _serializers) {
            co_await serializer(entity, _registry, entity_def);
        }
        co_return entity_def;
    }
}