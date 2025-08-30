#pragma once

#include <cstdint>
#include <type_traits>

#include "proto/ECS/components/transform_component.pb.h"
#include "proto/ECS/components/visual_component.pb.h"
#include "proto/ECS/components/camera_component.pb.h"
#include "proto/ECS/components/health_component.pb.h"
#include "proto/ECS/components/input_component.pb.h"
#include "proto/ECS/components/light_component.pb.h"
#include "proto/ECS/components/network_component.pb.h"
#include "proto/ECS/components/script_component.pb.h"
#include "proto/ECS/components/terrain_component.pb.h"

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
        proto::ecs::TransformComponent,
        proto::ecs::VisualComponent,
        proto::ecs::CameraComponent,
        proto::ecs::HealthComponent,
        proto::ecs::InputComponent,
        proto::ecs::LightComponent,
        proto::ecs::ScriptComponent,
        proto::ecs::TerrainComponent
    >;

} // namespace ecs
