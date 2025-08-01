#pragma once

#include "math/math.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"

namespace astre::ecs::system
{
    class TransformSystem : public System<TransformComponent>
    {
    public:
        static constexpr math::Vec3 BASE_FORWARD_DIRECTION = math::Vec3(0.0f, 0.0f, -1.0f);
        static constexpr math::Vec3 BASE_UP_DIRECTION = math::Vec3(0.0f, 1.0f, 0.0f);

        TransformSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context);
        
        inline TransformSystem(TransformSystem && other)
            : System(std::move(other))
        {}

        TransformSystem & operator=(TransformSystem && other) = delete;

        TransformSystem(const TransformSystem &) = delete;
        TransformSystem & operator=(const TransformSystem &) = delete;


        ~TransformSystem() = default;

        asio::awaitable<void> run();
    };
}