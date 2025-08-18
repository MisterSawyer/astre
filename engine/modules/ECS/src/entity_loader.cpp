#include "ecs/entity_loader.hpp"

namespace astre::ecs
{
    template<class ComponentType>
    static ComponentLoader _constructComponentLoader(
            std::function<bool(const EntityDefinition*)> has_component,
            std::function<const ComponentType &(const EntityDefinition*)> get_component) 
    {
        return 
            [has_component, get_component]
            (const EntityDefinition & entity_def, Entity entity, Registry & registry) -> asio::awaitable<void>
            {
                if (!has_component(&entity_def)) co_return;
                
                ComponentType component;
                component.CopyFrom(get_component(&entity_def));
                co_await registry.addComponent(entity, std::move(component));
            };
    }

    EntityLoader::EntityLoader()
    {
        _registerComponentLoader("TransformComponent", 
            _constructComponentLoader<TransformComponent>(
                &EntityDefinition::has_transform, &EntityDefinition::transform)
        );

        _registerComponentLoader("VisualComponent",
            _constructComponentLoader<VisualComponent>(
                &EntityDefinition::has_visual, &EntityDefinition::visual)
        );

        _registerComponentLoader("InputComponent",
            _constructComponentLoader<InputComponent>(
                &EntityDefinition::has_input, &EntityDefinition::input)
        );

        _registerComponentLoader("HealthComponent", 
            _constructComponentLoader<HealthComponent>(
                &EntityDefinition::has_health, &EntityDefinition::health)
        );

        _registerComponentLoader("CameraComponent", 
            _constructComponentLoader<CameraComponent>(
                &EntityDefinition::has_camera, &EntityDefinition::camera)
        );

        _registerComponentLoader("TerrainComponent", 
            _constructComponentLoader<TerrainComponent>(
                &EntityDefinition::has_terrain, &EntityDefinition::terrain)
        );

        _registerComponentLoader("LightComponent", 
            _constructComponentLoader<LightComponent>(
                &EntityDefinition::has_light, &EntityDefinition::light)
        );

        _registerComponentLoader("ScriptComponent", 
            _constructComponentLoader<ScriptComponent>(
                &EntityDefinition::has_script, &EntityDefinition::script)
        );
    }

    void EntityLoader::_registerComponentLoader(const std::string& name, ComponentLoader loader) 
    {
        _loaders[name] = std::move(loader);
    }

    asio::awaitable<void> EntityLoader::loadEntity(const EntityDefinition & entity_def, Entity entity, Registry & registry) const
    {
        for (const auto& [name, loader] : _loaders)
        {
            co_await loader(entity_def, entity, registry);
        }
    }
}