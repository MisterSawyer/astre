#pragma once

#include <asio.hpp>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "proto/ECS/entity_definition.pb.h"

namespace astre::loader
{
    class EntitySerializer
    {
    public:
        EntitySerializer() = default;
        inline EntitySerializer(EntitySerializer && other) = default;
        inline EntitySerializer& operator=(EntitySerializer && other) = default;
        EntitySerializer(const EntitySerializer &) = delete;
        EntitySerializer& operator=(const EntitySerializer &) = delete;

        asio::awaitable<proto::ecs::EntityDefinition> serializeEntity(const ecs::Entity entity, const ecs::Registry & registry) const;
    };
}
