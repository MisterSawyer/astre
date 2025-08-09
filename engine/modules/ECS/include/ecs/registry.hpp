#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>

#include "async/async.hpp"
#include "process/process.hpp"

#include "ecs/entity.hpp"
#include "ecs/entity_manager.hpp"
#include "ecs/component_type.hpp"
#include "ecs/component_manager.hpp"

namespace astre::ecs
{
    class Registry
    {
        public:
            Registry(process::IProcess::execution_context_type & execution_context);
            Registry(Registry && other);
            Registry& operator=(Registry && other);

            Registry(const Registry& other) = delete;
            Registry& operator=(const Registry& other) = delete;

            inline ~Registry() = default;

            asio::awaitable<std::optional<Entity>> createEntity(std::string name);
            asio::awaitable<void> destroyEntity(Entity entity);
            
            std::optional<Entity> getEntity(std::string name) const;
            asio::awaitable<std::optional<std::string>> getName(Entity entity) const;

            template<class ComponentType>
            inline asio::awaitable<void> addComponent(Entity entity, ComponentType&& component)
            {
                co_return _components.addComponent<ComponentType>(entity, std::move(component));
            }

            template<class ComponentType, class ... ComponentArgs>
            inline asio::awaitable<void> addComponent(Entity entity, ComponentArgs&&... args)
            {
                co_return _components.addComponent<ComponentType>(entity, ComponentType(std::forward<ComponentArgs>(args)...));
            }

            template<class ComponentType>
            asio::awaitable<bool> hasComponent(const Entity entity) const
            {
                co_return _entities.getComponentMask(entity).test(ComponentTypesList::template getTypeID<ComponentType>());
            }

            template<class ... ComponentTypes, class F>
            void runOnSingleWithComponents(const Entity entity, F && callable) 
            {
                if(_entities.entityExists(entity) == false) return;
                
                const auto& mask = _entities.getComponentMask(entity);
                const bool has_all_components = (... && mask.test(ComponentTypesList::template getTypeID<ComponentTypes>()));
                if (has_all_components)
                    callable(entity, (*_components.getComponent<ComponentTypes>(entity))...);
            }

            template<class ... ComponentTypes, class F>
            void runOnSingleWithComponents(const Entity entity, F && callable) const
            {
                if(_entities.entityExists(entity) == false) return;

                const auto& mask = _entities.getComponentMask(entity);
                const bool has_all_components = (... && mask.test(ComponentTypesList::template getTypeID<ComponentTypes>()));
                if (has_all_components)
                    callable(entity, (*_components.getComponent<ComponentTypes>(entity))...);
            }

            template<class ... ComponentTypes, class F>
            void runOnAllWithComponents(F && callable) 
            {
                for(auto & [entity, mask] : _entities.getAllEntities())
                {
                    const bool has_all_components = (... && mask.test(ComponentTypesList::template getTypeID<ComponentTypes>()));

                    if (has_all_components)
                        callable(entity, (*_components.getComponent<ComponentTypes>(entity))...);   
                }
            }

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            EntityManager _entities;
            ComponentManager _components;

            absl::flat_hash_map<std::string, Entity> _names_to_entities;
            absl::flat_hash_map<Entity, std::string> _entities_to_names;
    };   
}