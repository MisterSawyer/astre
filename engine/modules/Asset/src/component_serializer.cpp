#include "asset/component_serializer.hpp"

namespace astre::asset
{
    template<class ComponentType>
    static ComponentSerializer _constructComponentSerializer(std::function<ComponentType *(ecs::EntityDefinition *)> mutable_component)
    {
        return 
            [&mutable_component]
            (const ecs::Entity entity, const ecs::Registry & registry, ecs::EntityDefinition & entity_def)
            {
                if (!registry.hasComponent<ComponentType>(entity)) return;

                const ComponentType * const component = registry.getComponent<ComponentType>(entity);
                mutable_component(&entity_def)->CopyFrom(*component);
            };
    }

    ComponentSerializerRegistry::ComponentSerializerRegistry()
    {
        registerSerializer("TransformComponent",
            _constructComponentSerializer<ecs::TransformComponent>(&ecs::EntityDefinition::mutable_transform) 
        );

        registerSerializer("VisualComponent", 
            _constructComponentSerializer<ecs::VisualComponent>(&ecs::EntityDefinition::mutable_visual)
        );

        registerSerializer("InputComponent",
            _constructComponentSerializer<ecs::InputComponent>(&ecs::EntityDefinition::mutable_input)
        );

        registerSerializer("HealthComponent",
            _constructComponentSerializer<ecs::HealthComponent>(&ecs::EntityDefinition::mutable_health)
        );

        registerSerializer("CameraComponent", 
            _constructComponentSerializer<ecs::CameraComponent>(&ecs::EntityDefinition::mutable_camera)
        );

        registerSerializer("TerrainComponent", 
            _constructComponentSerializer<ecs::TerrainComponent>(&ecs::EntityDefinition::mutable_terrain)
        );

        registerSerializer("LightComponent", 
            _constructComponentSerializer<ecs::LightComponent>(&ecs::EntityDefinition::mutable_light)
        );

        registerSerializer("ScriptComponent", 
            _constructComponentSerializer<ecs::ScriptComponent>(&ecs::EntityDefinition::mutable_script)
        );
    }

    void ComponentSerializerRegistry::registerSerializer(const std::string& name, ComponentSerializer serializer)
    {
        _serializers[name] = std::move(serializer);
    }

    ecs::EntityDefinition ComponentSerializerRegistry::serializeEntity(const ecs::Entity entity, const ecs::Registry & registry) const {
        ecs::EntityDefinition entity_def;
        for (const auto& [name, serializer] : _serializers) {
            serializer(entity, registry, entity_def);
        }
        return entity_def;
    }
}