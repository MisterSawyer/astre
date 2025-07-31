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
        _components(_entities, std::move(other._components)),
        _names_to_entities(std::move(other._names_to_entities)),
        _entities_to_names(std::move(other._entities_to_names))
    {}

    Registry& Registry::operator=(Registry && other)
    {
        if(this == &other) return *this;

        _async_context = std::move(other._async_context);
        _entities = std::move(other._entities);
        _components = std::move(other._components);
        _names_to_entities = std::move(other._names_to_entities);
        _entities_to_names = std::move(other._entities_to_names);
        
        return *this;
    }

    asio::awaitable<std::optional<Entity>> Registry::createEntity(std::string name)
    {
        co_await _async_context.ensureOnStrand();

        if(_names_to_entities.contains(name))
        {
            spdlog::error("Entity with name {} already exists in the level", name); 
            co_return std::nullopt;
        }

        _names_to_entities.emplace(name, _entities.createEntity());
        _entities_to_names.emplace(_names_to_entities.at(name), name);

        co_return _names_to_entities.at(name);
    }

    asio::awaitable<void> Registry::destroyEntity(Entity entity)
    {
        co_await _async_context.ensureOnStrand();

        if(_entities_to_names.contains(entity))
        {
            _names_to_entities.erase(_entities_to_names.at(entity));
            _entities_to_names.erase(entity);

            _entities.destroyEntity(entity);
        }
    }

    asio::awaitable<std::optional<Entity>> Registry::getEntity(std::string name) const
    {
        co_await _async_context.ensureOnStrand();

        if(!_names_to_entities.contains(name)) co_return std::nullopt;
        co_return _names_to_entities.at(name);
    }

    asio::awaitable<std::optional<std::string>> Registry::getName(Entity entity) const
    {
        co_await _async_context.ensureOnStrand();

        if(!_entities_to_names.contains(entity)) co_return std::nullopt;
        co_return _entities_to_names.at(entity);
    }
}