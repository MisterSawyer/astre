#pragma once

#include "math/math.hpp"
#include "render/render.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/light_component.pb.h"

namespace astre::ecs::system
{
    class LightSystem : public System<LightComponent>
    {
    public:
        static constexpr const uint16_t MAX_LIGHTS = 256;
        static constexpr const uint16_t MAX_SHADOW_CASTERS = 16;

        LightSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        ~LightSystem() = default;

        asio::awaitable<void> run(render::Frame & frame);
    };
}