#pragma once

#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

#include "loader/loader_interface.hpp"

namespace astre::loader
{   
    /*
    * Load entity, from entity definition into the entity in registry
    */
    class EntityLoader : public ILoader
    {
    public:
        EntityLoader(ecs::Registry & registry);
        inline EntityLoader(EntityLoader && other) = default;
        inline EntityLoader& operator=(EntityLoader && other) = default;
        EntityLoader(const EntityLoader &) = delete;
        EntityLoader& operator=(const EntityLoader &) = delete;

        asio::awaitable<void> loadEntity(const proto::ecs::EntityDefinition & entity_def) const;

    private:
        /**
        * Load component, from entity definition into the entity in registry, 
        */
        using ComponentLoader = std::function<asio::awaitable<void>(const proto::ecs::EntityDefinition &, ecs::Entity, ecs::Registry &)>;

        void _registerComponentLoader(const std::string& name, ComponentLoader loader);
        
        ecs::Registry & _registry;

        absl::flat_hash_map<std::string, ComponentLoader> _loaders;
    };
}