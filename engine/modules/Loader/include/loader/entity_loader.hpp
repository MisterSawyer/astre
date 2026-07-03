#pragma once

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

    private:
        ecs::Registry & _registry;
    };
}
