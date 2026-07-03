#include "loader/entity_loader.hpp"

#include <spdlog/spdlog.h>

namespace astre::loader
{
    EntityLoader::EntityLoader(ecs::Registry & registry)
    : _registry(registry)
    {}

    asio::awaitable<bool> EntityLoader::load(const proto::ecs::EntityDefinition & entity_def) const
    {
        ecs::Entity id = entity_def.id();

        // if entity already present skip loading
        if(co_await _registry.entityExists(entity_def.id())) co_return true;

        // spawn entity
        auto entity_id_res = co_await _registry.spawnEntity(entity_def);
        if(entity_id_res.has_value() == false)
        {
            spdlog::error("[entity-loader] Failed to spawn entity");
            co_return false;
        }
        else
        {
            id = entity_id_res.value();
        }

        // load entity components
        if(entity_def.has_transform())
        {
            co_await _registry.addComponent<proto::ecs::TransformComponent>(id, entity_def.transform());
        }

        if(entity_def.has_visual())
        {
            co_await _registry.addComponent<proto::ecs::VisualComponent>(id, entity_def.visual());
        }

        if(entity_def.has_input())
        {
            co_await _registry.addComponent<proto::ecs::InputComponent>(id, entity_def.input());
        }

        if(entity_def.has_health())
        {
            co_await _registry.addComponent<proto::ecs::HealthComponent>(id, entity_def.health());
        }

        if(entity_def.has_camera())
        {
            co_await _registry.addComponent<proto::ecs::CameraComponent>(id, entity_def.camera());
        }

        if(entity_def.has_terrain())
        {
            co_await _registry.addComponent<proto::ecs::TerrainComponent>(id, entity_def.terrain());
        }

        if(entity_def.has_light())
        {
            co_await _registry.addComponent<proto::ecs::LightComponent>(id, entity_def.light());
        }

        if(entity_def.has_script())
        {
            co_await _registry.addComponent<proto::ecs::ScriptComponent>(id, entity_def.script());
        }

        spdlog::debug("[entity-loader] Entity {} loaded", id);
        co_return true;
    }
}
