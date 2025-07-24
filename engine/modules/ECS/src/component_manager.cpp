#include "ecs/component_manager.hpp"

namespace astre::ecs
{
    ComponentManager::ComponentManager(EntityManager& entity_manager) 
    : _entity_manager(&entity_manager)
    {}

    ComponentManager::ComponentManager(EntityManager& entity_manager, ComponentManager && other)
    : _entity_manager(&entity_manager),
      _storages(std::move(other._storages))
    {
        other._entity_manager = nullptr;
    }

    ComponentManager::ComponentManager(ComponentManager&& other)
    : _entity_manager(other._entity_manager),
      _storages(std::move(other._storages))
    {
        other._entity_manager = nullptr;
    }

    ComponentManager& ComponentManager::operator=(ComponentManager&& other)
    {
        if (this != &other)
        {
            _entity_manager = std::move(other._entity_manager);
            _storages = std::move(other._storages);
            other._entity_manager = nullptr;
        }
        return *this;
    }
}