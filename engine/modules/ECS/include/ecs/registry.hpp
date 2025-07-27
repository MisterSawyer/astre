#pragma once

#include <optional>

#include "ecs/entity.hpp"
#include "ecs/entity_manager.hpp"
#include "ecs/component_type.hpp"
#include "ecs/component_manager.hpp"

namespace astre::ecs
{
    class Registry
    {
        public:
            Registry();
            Registry(Registry && other);
            Registry& operator=(Registry && other);

            Registry(const Registry& other) = delete;
            Registry& operator=(const Registry& other) = delete;

            inline ~Registry() = default;

            template<class ComponentType>
            inline auto addComponent(Entity entity, ComponentType&& component)
            {
                return _components.addComponent<ComponentType>(entity, std::move(component));
            }

            template<class ComponentType, class ... ComponentArgs>
            inline auto addComponent(Entity entity, ComponentArgs&&... args)
            {
                return _components.addComponent<ComponentType>(entity, ComponentType(std::forward<ComponentArgs>(args)...));
            }

            template<class ComponentType>
            bool hasComponent(const Entity entity) const
            {
                return _entities.getComponentMask(entity).test(ComponentTypesList::template getTypeID<ComponentType>());
            }

            template<typename ComponentType>
            ComponentType * getComponent(const Entity entity) 
            { 
                return _components.getComponent<ComponentType>(entity);
            }

            template<typename ComponentType>
            const ComponentType * getComponent(const Entity entity) const
            { 
                return _components.getComponent<ComponentType>(entity);
            }

            std::optional<Entity> createEntity(std::string name);
            void destroyEntity(Entity entity);
            
            std::optional<Entity> getEntity(std::string name) const;
            std::optional<std::string> getName(Entity entity) const;

            const absl::flat_hash_map<Entity, std::bitset<MAX_COMPONENT_TYPES>> & getAllEntities() const { return _entities.getAllEntities(); }

        private:
            EntityManager _entities;
            ComponentManager _components;

            absl::flat_hash_map<std::string, Entity> _names_to_entities;
            absl::flat_hash_map<Entity, std::string> _entities_to_names;
    };   
}