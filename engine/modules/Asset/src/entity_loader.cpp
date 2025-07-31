#include "asset/entity_loader.hpp"

namespace astre::asset
{
    template<class ComponentType>
    static ComponentLoader _constructComponentLoader(
            std::function<bool(const ecs::EntityDefinition*)> has_component,
            std::function<const ComponentType &(const ecs::EntityDefinition*)> get_component) 
    {
        return 
            [has_component, get_component]
            (const ecs::EntityDefinition & entity_def, ecs::Entity entity, ecs::Registry & registry) -> asio::awaitable<void>
            {
                if (!has_component(&entity_def)) co_return;
                
                ComponentType component;
                component.CopyFrom(get_component(&entity_def));
                co_await registry.addComponent(entity, std::move(component));
            };
    }

    EntityLoader::EntityLoader(ecs::Registry & registry)
    : _registry(registry)
    {
        registerComponentLoader("TransformComponent", 
            _constructComponentLoader<ecs::TransformComponent>(
                &ecs::EntityDefinition::has_transform, &ecs::EntityDefinition::transform)
        );

        registerComponentLoader("VisualComponent",
            _constructComponentLoader<ecs::VisualComponent>(
                &ecs::EntityDefinition::has_visual, &ecs::EntityDefinition::visual)
        );

        registerComponentLoader("InputComponent",
            _constructComponentLoader<ecs::InputComponent>(
                &ecs::EntityDefinition::has_input, &ecs::EntityDefinition::input)
        );

        registerComponentLoader("HealthComponent", 
            _constructComponentLoader<ecs::HealthComponent>(
                &ecs::EntityDefinition::has_health, &ecs::EntityDefinition::health)
        );

        registerComponentLoader("CameraComponent", 
            _constructComponentLoader<ecs::CameraComponent>(
                &ecs::EntityDefinition::has_camera, &ecs::EntityDefinition::camera)
        );

        registerComponentLoader("TerrainComponent", 
            _constructComponentLoader<ecs::TerrainComponent>(
                &ecs::EntityDefinition::has_terrain, &ecs::EntityDefinition::terrain)
        );

        registerComponentLoader("LightComponent", 
            _constructComponentLoader<ecs::LightComponent>(
                &ecs::EntityDefinition::has_light, &ecs::EntityDefinition::light)
        );

        registerComponentLoader("ScriptComponent", 
            _constructComponentLoader<ecs::ScriptComponent>(
                &ecs::EntityDefinition::has_script, &ecs::EntityDefinition::script)
        );
    }

    void EntityLoader::registerComponentLoader(const std::string& name, ComponentLoader loader) 
    {
        _loaders[name] = std::move(loader);
    }

    asio::awaitable<void> EntityLoader::loadEntity(const ecs::EntityDefinition & entity_def, ecs::Entity entity) const 
    {
        for (const auto& [name, loader] : _loaders)
        {
            co_await loader(entity_def, entity, _registry);
        }
    }

}