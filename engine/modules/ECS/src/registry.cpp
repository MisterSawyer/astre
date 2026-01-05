#include "ecs/registry.hpp"

#include <spdlog/spdlog.h>

namespace astre::ecs
{
    Registry::Registry(process::IProcess::execution_context_type & execution_context)
    :   _async_context(execution_context),    
        _entities(),
        _components(_entities)
    {}

    Registry::Registry(Registry && other)
    :   _async_context(std::move(other._async_context)),
        _entities(std::move(other._entities)),
        _components(_entities, std::move(other._components))
    {}

    Registry& Registry::operator=(Registry && other)
    {
        if(this == &other) return *this;

        _async_context = std::move(other._async_context);
        _entities = std::move(other._entities);
        _components = std::move(other._components);
        
        return *this;
    }

    asio::awaitable<bool> Registry::entityExists(Entity entity) const
    {
        co_await _async_context.ensureOnStrand();
        co_return _entities.entityExists(entity);
    }

    asio::awaitable<std::optional<Entity>> Registry::spawnEntity(const proto::ecs::EntityDefinition & entity_def)
    {
        co_await _async_context.ensureOnStrand();
        const auto res_id = _entities.spawnEntity(entity_def.id());

        if(!res_id) 
        {
            spdlog::error("Failed to create entity");
            co_return std::nullopt;
        }

        _entity_names[entity_def.id()] = entity_def.name();

        co_return res_id;
    }

    asio::awaitable<void> Registry::destroyEntity(Entity entity)
    {
        co_await _async_context.ensureOnStrand();
        _entity_names.erase(entity);
        _entities.destroyEntity(entity);
    }
}