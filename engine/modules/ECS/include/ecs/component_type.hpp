#pragma once

#include <cstdint>
#include <type_traits>

#include "generated/ECS/proto/components/transform_component.pb.h"
#include "generated/ECS/proto/components/visual_component.pb.h"
#include "generated/ECS/proto/components/camera_component.pb.h"
#include "generated/ECS/proto/components/health_component.pb.h"
#include "generated/ECS/proto/components/input_component.pb.h"
#include "generated/ECS/proto/components/light_component.pb.h"
#include "generated/ECS/proto/components/network_component.pb.h"
#include "generated/ECS/proto/components/script_component.pb.h"
#include "generated/ECS/proto/components/terrain_component.pb.h"

namespace astre::ecs {

    template<typename T, typename... Ts>
    struct index_of;

    template<typename T, typename... Rest>
    struct index_of<T, T, Rest...>
    {
        static constexpr std::size_t value = 0;
    };

    template<typename T, typename First, typename... Rest>
    struct index_of<T, First, Rest...>
    {
        static constexpr std::size_t value = std::is_same_v<T, First> ? 0 : 1 + index_of<T, Rest...>::value;
    };

    template<typename T>
    struct index_of<T> {
        static_assert(sizeof(T) == 0, "Component type not registered in ComponentTypesList.");
    };

    template<typename... ComponentList>
    struct ComponentTypes
    {
        template<typename Component>
        static consteval std::uint32_t getTypeID()
        {
            return static_cast<std::uint32_t>(index_of<Component, ComponentList...>::value);
        }
    };

    using ComponentTypesList = ComponentTypes<
        TransformComponent,
        VisualComponent,
        CameraComponent,
        HealthComponent,
        InputComponent,
        LightComponent,
        ScriptComponent,
        TerrainComponent
    >;

} // namespace ecs
