#include <spdlog/spdlog.h>

#include "loader/entity_serializer.hpp"

namespace astre::loader
{
    template<class ComponentType>
    static ComponentSerializer _constructComponentSerializer(std::function<ComponentType *(proto::ecs::EntityDefinition *)> mutable_component)
    {
        return 
            [mutable_component]
            (const ecs::Entity entity, const ecs::Registry & registry, proto::ecs::EntityDefinition & entity_def) -> asio::awaitable<void>
            {
                if (!(co_await registry.hasComponent<ComponentType>(entity))) co_return;

                auto* target = mutable_component(&entity_def);
                if (!target)
                {
                    spdlog::error("Entity {} has null mutable component", entity);
                    throw std::runtime_error("mutable_component returned nullptr");
                }

                registry.runOnSingleWithComponents<ComponentType>(entity, 
                [&target](const ecs::Entity entity, const ComponentType & component)
                {
                    target->CopyFrom(component);
                });

                co_return; 
            };
    }

    EntitySerializer::EntitySerializer()
    {
        _registerComponentSerializer("TransformComponent",
            _constructComponentSerializer<proto::ecs::TransformComponent>(&proto::ecs::EntityDefinition::mutable_transform) 
        );

        _registerComponentSerializer("VisualComponent", 
            _constructComponentSerializer<proto::ecs::VisualComponent>(&proto::ecs::EntityDefinition::mutable_visual)
        );

        _registerComponentSerializer("InputComponent",
            _constructComponentSerializer<proto::ecs::InputComponent>(&proto::ecs::EntityDefinition::mutable_input)
        );

        _registerComponentSerializer("HealthComponent",
            _constructComponentSerializer<proto::ecs::HealthComponent>(&proto::ecs::EntityDefinition::mutable_health)
        );

        _registerComponentSerializer("CameraComponent", 
            _constructComponentSerializer<proto::ecs::CameraComponent>(&proto::ecs::EntityDefinition::mutable_camera)
        );

        _registerComponentSerializer("TerrainComponent", 
            _constructComponentSerializer<proto::ecs::TerrainComponent>(&proto::ecs::EntityDefinition::mutable_terrain)
        );

        _registerComponentSerializer("LightComponent", 
            _constructComponentSerializer<proto::ecs::LightComponent>(&proto::ecs::EntityDefinition::mutable_light)
        );

        _registerComponentSerializer("ScriptComponent", 
            _constructComponentSerializer<proto::ecs::ScriptComponent>(&proto::ecs::EntityDefinition::mutable_script)
        );
    }

    void EntitySerializer::_registerComponentSerializer(const std::string& name, ComponentSerializer serializer)
    {
        _serializers[name] = std::move(serializer);
    }

    asio::awaitable<proto::ecs::EntityDefinition> EntitySerializer::serializeEntity(const ecs::Entity entity, const ecs::Registry & registry) const {
        proto::ecs::EntityDefinition entity_def;
        auto name_res = co_await registry.getName(entity);
        if (name_res.has_value() == false) {
            spdlog::error("Entity {} has no name", entity);
            throw std::runtime_error("Entity has no name");
        }
        entity_def.set_name(name_res.value());
        for (const auto& [name, serializer] : _serializers) {
            co_await serializer(entity, registry, entity_def);
        }
        co_return entity_def;
    }
}