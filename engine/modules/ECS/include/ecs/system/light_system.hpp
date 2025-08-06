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
        using Reads = std::tuple<TransformComponent>;
        using Writes = std::tuple<CameraComponent>;

        static constexpr uint16_t MAX_LIGHTS = 256;
        static constexpr uint16_t MAX_SHADOW_CASTERS = 16;

        LightSystem(Registry & registry);

        inline LightSystem(LightSystem && other)
            : System(std::move(other))
        {}

        LightSystem & operator=(LightSystem && other) = delete;

        LightSystem(const LightSystem &) = delete;
        LightSystem & operator=(const LightSystem &) = delete;

        ~LightSystem() = default;

        asio::awaitable<void> run(float dt, render::Frame & frame);

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }
    };
}