#include "loader/entity_loader.hpp"

namespace astre::loader
{
    template<class ComponentType>
    static EntityLoader::ComponentLoader _constructComponentLoader(
            std::function<bool(const proto::ecs::EntityDefinition*)> has_component,
            std::function<const ComponentType &(const proto::ecs::EntityDefinition*)> get_component) 
    {
        return 
            [has_component, get_component]
            (const proto::ecs::EntityDefinition & entity_def, ecs::Entity entity, ecs::Registry & registry) -> asio::awaitable<void>
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
        _registerComponentLoader("TransformComponent", 
            _constructComponentLoader<proto::ecs::TransformComponent>(
                &proto::ecs::EntityDefinition::has_transform, &proto::ecs::EntityDefinition::transform)
        );

        _registerComponentLoader("VisualComponent",
            _constructComponentLoader<proto::ecs::VisualComponent>(
                &proto::ecs::EntityDefinition::has_visual, &proto::ecs::EntityDefinition::visual)
        );

        _registerComponentLoader("InputComponent",
            _constructComponentLoader<proto::ecs::InputComponent>(
                &proto::ecs::EntityDefinition::has_input, &proto::ecs::EntityDefinition::input)
        );

        _registerComponentLoader("HealthComponent", 
            _constructComponentLoader<proto::ecs::HealthComponent>(
                &proto::ecs::EntityDefinition::has_health, &proto::ecs::EntityDefinition::health)
        );

        _registerComponentLoader("CameraComponent", 
            _constructComponentLoader<proto::ecs::CameraComponent>(
                &proto::ecs::EntityDefinition::has_camera, &proto::ecs::EntityDefinition::camera)
        );

        _registerComponentLoader("TerrainComponent", 
            _constructComponentLoader<proto::ecs::TerrainComponent>(
                &proto::ecs::EntityDefinition::has_terrain, &proto::ecs::EntityDefinition::terrain)
        );

        _registerComponentLoader("LightComponent", 
            _constructComponentLoader<proto::ecs::LightComponent>(
                &proto::ecs::EntityDefinition::has_light, &proto::ecs::EntityDefinition::light)
        );

        _registerComponentLoader("ScriptComponent", 
            _constructComponentLoader<proto::ecs::ScriptComponent>(
                &proto::ecs::EntityDefinition::has_script, &proto::ecs::EntityDefinition::script)
        );
    }

    void EntityLoader::_registerComponentLoader(const std::string& name, ComponentLoader loader) 
    {
        _loaders[name] = std::move(loader);
    }

    asio::awaitable<void> EntityLoader::load(const proto::ecs::EntityDefinition & entity_def) const
    {
        ecs::Entity id = entity_def.id();

        // if entity already present skip loading
        if(co_await _registry.entityExists(entity_def.id())) co_return;

        // spawn entity
        auto entity_id_res = co_await _registry.spawnEntity(entity_def);
        if(entity_id_res.has_value() == false)
        {
            spdlog::error("[entity-loader] Failed to spawn entity");
            co_return;
        }
        else
        {
            id = entity_id_res.value();
        }

        // load entity components
        for (const auto& [name, loader] : _loaders)
        {
            co_await loader(entity_def, id, _registry);
        }

        spdlog::debug("[entity-loader] Entity {} loaded", id);
    }
}