#pragma once

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"

namespace astre::ecs::system
{
    class TransformSystem : public System<astre::ecs::TransformComponent>
    {
    public:
        TransformSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        ~TransformSystem() = default;

        asio::awaitable<void> run();
    };
}