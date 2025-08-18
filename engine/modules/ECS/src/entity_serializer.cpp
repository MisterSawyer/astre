#include <spdlog/spdlog.h>

#include "ecs/entity_serializer.hpp"

namespace astre::ecs
{
    template<class ComponentType>
    static ComponentSerializer _constructComponentSerializer(std::function<ComponentType *(EntityDefinition *)> mutable_component)
    {
        return 
            [mutable_component]
            (const Entity entity, const Registry & registry, EntityDefinition & entity_def) -> asio::awaitable<void>
            {
                if (!(co_await registry.hasComponent<ComponentType>(entity))) co_return;

                auto* target = mutable_component(&entity_def);
                if (!target)
                {
                    spdlog::error("Entity {} has null mutable component", entity);
                    throw std::runtime_error("mutable_component returned nullptr");
                }

                registry.runOnSingleWithComponents<ComponentType>(entity, 
                [&target](const Entity entity, const ComponentType & component)
                {
                    target->CopyFrom(component);
                });

                co_return; 
            };
    }

    EntitySerializer::EntitySerializer()
    {
        _registerComponentSerializer("TransformComponent",
            _constructComponentSerializer<TransformComponent>(&EntityDefinition::mutable_transform) 
        );

        _registerComponentSerializer("VisualComponent", 
            _constructComponentSerializer<VisualComponent>(&EntityDefinition::mutable_visual)
        );

        _registerComponentSerializer("InputComponent",
            _constructComponentSerializer<InputComponent>(&EntityDefinition::mutable_input)
        );

        _registerComponentSerializer("HealthComponent",
            _constructComponentSerializer<HealthComponent>(&EntityDefinition::mutable_health)
        );

        _registerComponentSerializer("CameraComponent", 
            _constructComponentSerializer<CameraComponent>(&EntityDefinition::mutable_camera)
        );

        _registerComponentSerializer("TerrainComponent", 
            _constructComponentSerializer<TerrainComponent>(&EntityDefinition::mutable_terrain)
        );

        _registerComponentSerializer("LightComponent", 
            _constructComponentSerializer<LightComponent>(&EntityDefinition::mutable_light)
        );

        _registerComponentSerializer("ScriptComponent", 
            _constructComponentSerializer<ScriptComponent>(&EntityDefinition::mutable_script)
        );
    }

    void EntitySerializer::_registerComponentSerializer(const std::string& name, ComponentSerializer serializer)
    {
        _serializers[name] = std::move(serializer);
    }

    asio::awaitable<EntityDefinition> EntitySerializer::serializeEntity(const Entity entity, const Registry & registry) const {
        EntityDefinition entity_def;
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