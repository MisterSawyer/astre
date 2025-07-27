#pragma once

#include "math/math.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"

namespace astre::ecs::system
{
    class TransformSystem : public System<TransformComponent>
    {
    public:
        constexpr static const math::Vec3 BASE_FORWARD_DIRECTION = math::Vec3(0.0f, 0.0f, -1.0f);
        constexpr static const math::Vec3 BASE_UP_DIRECTION = math::Vec3(0.0f, 1.0f, 0.0f);

        TransformSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        ~TransformSystem() = default;

        asio::awaitable<void> run();
    };
}