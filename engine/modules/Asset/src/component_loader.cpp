#include "asset/component_loader.hpp"

namespace astre::asset
{
    template<class ComponentType>
    static ComponentLoader _constructComponentLoader(
            std::function<bool(const ecs::EntityDefinition*)> has_component,
            std::function<const ComponentType &(const ecs::EntityDefinition*)> get_component) 
    {
        return 
            [&has_component, &get_component]
            (const ecs::EntityDefinition & entity_def, ecs::Entity entity, ecs::Registry & registry) 
            {
                if (!has_component(&entity_def)) return;
                
                ComponentType component;
                component.CopyFrom(get_component(&entity_def));
                registry.addComponent(entity, std::move(component));
            };
    }

    ComponentLoaderRegistry::ComponentLoaderRegistry()
    {
        registerLoader("TransformComponent", 
            _constructComponentLoader<ecs::TransformComponent>(
                &ecs::EntityDefinition::has_transform, &ecs::EntityDefinition::transform)
        );

        registerLoader("VisualComponent",
            _constructComponentLoader<ecs::VisualComponent>(
                &ecs::EntityDefinition::has_visual, &ecs::EntityDefinition::visual)
        );

        registerLoader("InputComponent",
            _constructComponentLoader<ecs::InputComponent>(
                &ecs::EntityDefinition::has_input, &ecs::EntityDefinition::input)
        );

        registerLoader("HealthComponent", 
            _constructComponentLoader<ecs::HealthComponent>(
                &ecs::EntityDefinition::has_health, &ecs::EntityDefinition::health)
        );

        registerLoader("CameraComponent", 
            _constructComponentLoader<ecs::CameraComponent>(
                &ecs::EntityDefinition::has_camera, &ecs::EntityDefinition::camera)
        );

        registerLoader("TerrainComponent", 
            _constructComponentLoader<ecs::TerrainComponent>(
                &ecs::EntityDefinition::has_terrain, &ecs::EntityDefinition::terrain)
        );

        registerLoader("LightComponent", 
            _constructComponentLoader<ecs::LightComponent>(
                &ecs::EntityDefinition::has_light, &ecs::EntityDefinition::light)
        );

        registerLoader("ScriptComponent", 
            _constructComponentLoader<ecs::ScriptComponent>(
                &ecs::EntityDefinition::has_script, &ecs::EntityDefinition::script)
        );
    }

    void ComponentLoaderRegistry::registerLoader(const std::string& name, ComponentLoader loader) 
    {
        _loaders[name] = std::move(loader);
    }

    void ComponentLoaderRegistry::loadComponents(const ecs::EntityDefinition & entity_def, ecs::Entity entity, ecs::Registry & registry) const 
    {
        for (const auto& [name, loader] : _loaders)
        {
            loader(entity_def, entity, registry);
        }
    }

}