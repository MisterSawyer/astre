#include "ecs/entity_manager.hpp"

namespace astre::ecs
{
    EntityManager::EntityManager()
        :_next_entity(1)
    {}

    EntityManager::EntityManager(EntityManager && other)
        :   _next_entity(std::move(other._next_entity)),
            _masks(std::move(other._masks))
    {}

    EntityManager& EntityManager::operator=(EntityManager && other)
    {
        if (this != &other)
        {
            _next_entity = std::move(other._next_entity);
            _masks = std::move(other._masks);
        }
        return *this;
    }
    
    Entity EntityManager::createEntity() 
    {
        Entity entity = _next_entity++;
        _masks[entity] = std::bitset<MAX_COMPONENT_TYPES>();
        return entity;
    }

    void EntityManager::destroyEntity(Entity entity)
    {
        assert(_masks.contains(entity));
        _masks.erase(entity);
    }

    bool EntityManager::entityExists(Entity entity) const
    {
        return _masks.contains(entity);
    }

    void EntityManager::addComponentBit(Entity entity, uint32_t component_type_ID)
    {
        assert(component_type_ID < MAX_COMPONENT_TYPES);
        _masks[entity].set(component_type_ID);
    }
              
    void EntityManager::removeComponentBit(Entity entity, uint32_t component_type_ID)
    {
        assert(component_type_ID < MAX_COMPONENT_TYPES);
        _masks[entity].reset(component_type_ID);
    }

    const std::bitset<MAX_COMPONENT_TYPES>& EntityManager::getComponentMask(Entity entity) const 
    {
        return _masks.at(entity);
    }
            
    const absl::flat_hash_map<Entity, std::bitset<MAX_COMPONENT_TYPES>> & EntityManager::getAllEntities() const 
    {
        return _masks;
    }
}