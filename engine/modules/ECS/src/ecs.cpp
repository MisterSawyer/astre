#include "ecs/ecs.hpp"

#include <spdlog/spdlog.h>

namespace astre::ecs
{
    Registry::Registry()
    :   _entities(),
        _components(_entities)
    {}

    Registry::Registry(Registry && other)
    : _entities(std::move(other._entities)),
      _components(_entities, std::move(other._components)),
      _names_to_entities(std::move(other._names_to_entities)),
      _entities_to_names(std::move(other._entities_to_names))
    {}

    Registry& Registry::operator=(Registry && other)
    {
        if(this == &other) return *this;
        
        _entities = std::move(other._entities);
        _components = std::move(other._components);
        _names_to_entities = std::move(other._names_to_entities);
        _entities_to_names = std::move(other._entities_to_names);
        
        return *this;
    }

    std::optional<Entity> Registry::createEntity(std::string name)
    {
        if(_names_to_entities.contains(name))
        {
            spdlog::error("Entity with name {} already exists in the level", name); 
            return std::nullopt;
        }

        _names_to_entities.emplace(name, _entities.createEntity());
        _entities_to_names.emplace(_names_to_entities.at(name), name);

        return _names_to_entities.at(name);
    }
}