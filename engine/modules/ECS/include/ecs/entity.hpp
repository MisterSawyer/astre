#pragma once

#include <utility>
#include <cstdint>

#include "proto/ECS/entity_definition.pb.h"

namespace astre::ecs
{
    using Entity = std::uint64_t;

    constexpr Entity INVALID_ENTITY = 0;
    constexpr std::size_t MAX_COMPONENT_TYPES = 256;
}

namespace astre::proto::ecs
{
    inline bool operator==(const proto::ecs::EntityDefinition& lhs, const proto::ecs::EntityDefinition& rhs) 
    { return lhs.id() == rhs.id(); }

    template <typename H>
    inline H AbslHashValue(H h, const proto::ecs::EntityDefinition & entity_definition) {
        return H::combine(std::move(h), entity_definition.id());
    }
}