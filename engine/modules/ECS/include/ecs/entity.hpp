#pragma once

#include <utility>
#include <cstdint>

namespace astre::ecs
{
    using Entity = std::uint32_t;

    constexpr Entity INVALID_ENTITY = 0;
    constexpr std::size_t MAX_COMPONENT_TYPES = 256;
}