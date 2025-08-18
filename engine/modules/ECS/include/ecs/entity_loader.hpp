#pragma once

#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

namespace astre::ecs
{
    /**
     * Load component, from entity definition into the entity in registry, 
    */
    using ComponentLoader = std::function<asio::awaitable<void>(const EntityDefinition &, Entity, Registry &)>;
    
    class EntityLoader 
    {
    public:
        EntityLoader();
        inline EntityLoader(EntityLoader && other) = default;
        inline EntityLoader& operator=(EntityLoader && other) = default;
        EntityLoader(const EntityLoader &) = delete;
        EntityLoader& operator=(const EntityLoader &) = delete;

        asio::awaitable<void> loadEntity(const EntityDefinition & entity_def, Entity entity, Registry & registry) const;

    private:
        void _registerComponentLoader(const std::string& name, ComponentLoader loader);
        
        absl::flat_hash_map<std::string, ComponentLoader> _loaders;
    };
}