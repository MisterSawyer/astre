#pragma once

#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/ecs.hpp"

namespace astre::asset
{
    /**
     * Load component, for entity in registry, from entity definition
    */
    using ComponentLoader = std::function<asio::awaitable<void>(const ecs::EntityDefinition &, ecs::Entity, ecs::Registry &)>;
    
    class EntityLoader 
    {
    public:
        EntityLoader(ecs::Registry & registry);
        inline EntityLoader(EntityLoader && other) = default;
        inline EntityLoader& operator=(EntityLoader && other) = default;
        EntityLoader(const EntityLoader &) = delete;
        EntityLoader& operator=(const EntityLoader &) = delete;

        asio::awaitable<void> loadEntity(const ecs::EntityDefinition & entity_def, ecs::Entity entity) const;

    private:
        void registerComponentLoader(const std::string& name, ComponentLoader loader);
        
        ecs::Registry & _registry;
        absl::flat_hash_map<std::string, ComponentLoader> _loaders;
    };
}