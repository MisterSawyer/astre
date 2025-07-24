#pragma once

#include <bitset>
#include <absl/container/flat_hash_map.h>

#include "ecs/entity.hpp"

namespace astre::ecs
{
    class EntityManager 
    {
        public:
            EntityManager();
            EntityManager(EntityManager &&);
            EntityManager(const EntityManager &) = delete;
            EntityManager& operator=(EntityManager &&);
            EntityManager& operator=(const EntityManager &) = delete;
            ~EntityManager() = default;

            /**
             * @brief Creates a new entity.
             * 
             * @return The ID of the newly created entity.
             */
            Entity createEntity();

            /**
             * @brief Destroys an entity and its associated components.
             * 
             * @param entity The ID of the entity to destroy.
             */
            void destroyEntity(Entity entity);

            /**
             * @brief Checks if an entity exists.
             * 
             * @param entity The ID of the entity to check.
             * @return True if the entity exists, false otherwise.
             */
            bool entityExists(Entity entity) const;

            /**
             * @brief Adds a component type to an entity's component mask.
             * 
             * @param entity The ID of the entity to add the component to.
             * @param component_type_ID The ID of the component type to add.
             */
            void addComponentBit(Entity entity, uint32_t component_type_ID);
            
            /**
             * @brief Removes a component type from an entity's component mask.
             * 
             * @param entity The ID of the entity to remove the component from.
             * @param component_type_ID The ID of the component type to remove.
             */
            void removeComponentBit(Entity entity, uint32_t component_type_ID);

            /**
             * @brief Retrieves the component mask for an entity.
             * 
             * @param entity The ID of the entity to retrieve the component mask for.
             * @return The component mask for the entity.
             */
            const std::bitset<MAX_COMPONENT_TYPES>& getComponentMask(Entity entity) const;

            /**
             * @brief Retrieves all entities and their component masks.
             * 
             * @return A map of entities and their component masks.
             */
            const absl::flat_hash_map<Entity, std::bitset<MAX_COMPONENT_TYPES>> & getAllEntities() const;

    private:
            Entity _next_entity;
            absl::flat_hash_map<Entity, std::bitset<MAX_COMPONENT_TYPES>> _masks;
    };
}