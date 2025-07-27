#pragma once

#include <typeindex>
#include <memory>

#include <absl/container/flat_hash_map.h>

#include "type/type.hpp"

#include "ecs/entity.hpp"
#include "ecs/entity_manager.hpp"
#include "ecs/component_type.hpp"

namespace astre::ecs
{
    /**
     * @brief Interface for component storage.
     */
    class IComponentStorage : public type::InterfaceBase
    {
        public:
            virtual ~IComponentStorage() = default;
    };

    /**
     * @brief Storage for components of a specific type.
     * 
     * This class manages the storage and retrieval of components associated with
     * entities in the ECS (Entity-Component-System) architecture.
     * 
     * @tparam Component The type of the component to store.
     */
    template<typename Component>
    class ComponentStorage : public IComponentStorage 
    {
        public:
            inline ComponentStorage() = default;
            ComponentStorage(const ComponentStorage&) = delete;
            ComponentStorage& operator=(const ComponentStorage&) = delete;
            inline ComponentStorage(ComponentStorage&&) = default;
            inline ComponentStorage& operator=(ComponentStorage&&) = default;
            inline ~ComponentStorage() = default;

            /**
             * @brief Adds a component to the storage.
             * 
             * @param entity The entity to which the component belongs.
             * 
             * @param component The component to add.
             */
            void add(Entity entity, Component component)
            {
                _components[entity] = component;
            }

            /**
             * @brief Retrieves a component from the storage.
             * 
             * @param entity The entity whose component to retrieve.
             * 
             * @return A pointer to the component, or nullptr if not found.
             */
            inline Component* get(Entity entity)
            {
                auto it = _components.find(entity);
                if (it != _components.end()) {
                    return &it->second;
                }
                return nullptr;
            }

            /**
             * @brief Retrieves a component from the storage.
             * 
             * @param entity The entity whose component to retrieve.
             * 
             * @return A pointer to the component, or nullptr if not found.
             */
            inline const Component* get(Entity entity) const
            {
                auto it = _components.find(entity);
                if (it != _components.end()) {
                    return &it->second;
                }
                return nullptr;
            }

        private:
            absl::flat_hash_map<Entity, Component> _components;
    };

    /**
     * @brief Manages components for entities.
     * 
     * This class is responsible for adding, retrieving, and managing components
     * associated with entities in the ECS (Entity-Component-System) architecture.
     */
    class ComponentManager
    {
        public:
            ComponentManager(EntityManager& entity_manager);
            ComponentManager(EntityManager& entity_manager, ComponentManager && other);

            ComponentManager(ComponentManager&&);
            ComponentManager& operator=(ComponentManager&&);

            ComponentManager(const ComponentManager&) = delete;
            ComponentManager& operator=(const ComponentManager&) = delete;

            ~ComponentManager() = default;

            /**
             * @brief Adds a component to an entity.
             * 
             * @tparam Component The type of the component to add.
             * 
             * @param entity The entity to which the component belongs.
             * 
             * @param component The component to add.
             */
            template<typename Component>
            void addComponent(Entity entity, Component component)
            {
                getOrCreateStorage<Component>()->add(entity, component);

                // Update entity mask
                uint32_t type_ID = ComponentTypesList::template getTypeID<Component>();
                assert(type_ID < MAX_COMPONENT_TYPES);
                assert(_entity_manager != nullptr);
                _entity_manager->addComponentBit(entity, type_ID);
            }

            /**
             * @brief Retrieves a component from an entity.
             * 
             * @tparam Component The type of the component to retrieve.
             * 
             * @param entity The entity whose component to retrieve.
             * 
             * @return A pointer to the component, or nullptr if not found.
             */
            template<typename Component>
            Component* getComponent(Entity entity)
            {
                auto storage = getStorage<Component>();
                if (storage) {
                    return storage->get(entity);
                }
                return nullptr;
            }

            /**
             * @brief Retrieves a component from an entity.
             * 
             * @tparam Component The type of the component to retrieve.
             * 
             * @param entity The entity whose component to retrieve.
             * 
             * @return A pointer to the component, or nullptr if not found.
             */
            template<typename Component>
            const Component*  getComponent(Entity entity) const
            {
                auto storage = getStorage<Component>();
                if (storage) {
                    return storage->get(entity);
                }
                return nullptr;
            }

        private:
            /**
             * @brief Retrieves the storage for a specific component type.
             * 
             * @tparam Component The type of the component.
             * 
             * @return A pointer to the storage, or nullptr if not found.
             */
            template<typename Component>
            ComponentStorage<Component>* getStorage()
            {
                auto it = _storages.find(std::type_index(typeid(Component)));
                if (it != _storages.end()) {
                    return static_cast<ComponentStorage<Component>*>(it->second.get());
                }
                return nullptr;
            }

            /**
             * @brief Retrieves the storage for a specific component type.
             * 
             * @tparam Component The type of the component.
             * 
             * @return A pointer to the storage, or nullptr if not found.
             */
            template<typename Component>
            const ComponentStorage<Component>*  getStorage() const
            {
                auto it = _storages.find(std::type_index(typeid(Component)));
                if (it != _storages.end()) {
                    return static_cast<const ComponentStorage<Component>*>(it->second.get());
                }
                return nullptr;
            }

            /**
             * @brief Retrieves or creates the storage for a specific component type.
             * 
             * @tparam Component The type of the component.
             * 
             * @return A pointer to the storage.
             */
            template<typename Component>
            ComponentStorage<Component>* getOrCreateStorage()
            {
                auto storage = getStorage<Component>();
                if (!storage) {
                    _storages[std::type_index(typeid(Component))] = std::make_unique<ComponentStorage<Component>>();
                    storage = static_cast<ComponentStorage<Component>*>(_storages[std::type_index(typeid(Component))].get());
                }
                return storage;
            }

            EntityManager* _entity_manager;
            absl::flat_hash_map<std::type_index, std::unique_ptr<IComponentStorage>> _storages;
    };
}
