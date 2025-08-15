#pragma once

#include <utility>
#include <cstdint>

#include "generated/ECS/proto/entity_definition.pb.h"

namespace astre::ecs
{
    using Entity = std::uint64_t;

    constexpr Entity INVALID_ENTITY = 0;
    constexpr std::size_t MAX_COMPONENT_TYPES = 256;

    inline bool operator==(const EntityDefinition& lhs, const EntityDefinition& rhs) 
    { return lhs.id() == rhs.id(); }

    template <typename H>
    inline H AbslHashValue(H h, const EntityDefinition & entity_definition) {
        return H::combine(std::move(h), entity_definition.id());
    }
}