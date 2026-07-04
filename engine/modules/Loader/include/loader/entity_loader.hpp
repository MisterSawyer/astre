#pragma once

#include <vector>

#include <asio.hpp>

#include "asset/concepts.hpp"
#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "proto/ECS/entity_definition.pb.h"

namespace astre::loader
{
    /*
    * Load entity, from entity definition into the entity in registry
    */
    class EntityLoader
    {
    public:
        EntityLoader(ecs::Registry & registry);
        inline EntityLoader(EntityLoader && other) = default;
        inline EntityLoader& operator=(EntityLoader && other) = default;
        EntityLoader(const EntityLoader &) = delete;
        EntityLoader& operator=(const EntityLoader &) = delete;

        asio::awaitable<bool> load(const proto::ecs::EntityDefinition & entity_def) const;
        // ponytail: batch = loop over single load; registry has no batched spawn.
        asio::awaitable<bool> load(const std::vector<proto::ecs::EntityDefinition> & entity_defs) const;
        asio::awaitable<bool> unload(const std::vector<ecs::Entity> & entities) const;
        asio::awaitable<bool> unload(ecs::Entity entity) const;
        ecs::Registry & registry() const { return _registry; }

    private:
        ecs::Registry & _registry;
    };
}
