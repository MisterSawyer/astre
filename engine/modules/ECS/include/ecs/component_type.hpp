#pragma once

#include <typeindex>
#include <cstdint>
#include <stdexcept>

namespace astre::ecs
{
    class ComponentTypeManager 
    {
    public:
        /**
         * @brief Retrieves the type ID for a given component type.
         * 
         * @tparam Component The component type.
         * 
         * @return The unique type ID for the component.
         */
        template<typename Component>
        constexpr inline static const uint32_t getTypeID() 
        {
            static const uint32_t id = _COUNTER++;
            assert(id < MAX_COMPONENT_TYPES);
            return id;
        }

    private:
        static inline uint32_t _COUNTER = 0;
    };
}