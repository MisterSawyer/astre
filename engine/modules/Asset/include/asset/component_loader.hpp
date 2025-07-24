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
    using ComponentLoader = std::function<void(const ecs::EntityDefinition &, ecs::Entity, ecs::Registry &)>;
    
    class ComponentLoaderRegistry 
    {
    public:
        ComponentLoaderRegistry();
        inline ComponentLoaderRegistry(ComponentLoaderRegistry && other) = default;
        inline ComponentLoaderRegistry& operator=(ComponentLoaderRegistry && other) = default;
        ComponentLoaderRegistry(const ComponentLoaderRegistry &) = delete;
        ComponentLoaderRegistry& operator=(const ComponentLoaderRegistry &) = delete;

        void loadComponents(const ecs::EntityDefinition & entity_def, ecs::Entity entity, ecs::Registry & registry) const;

    private:
        void registerLoader(const std::string& name, ComponentLoader loader);

        absl::flat_hash_map<std::string, ComponentLoader> _loaders;
    };
}