#pragma once

#include "script/script.hpp"
#include "ecs/system/system.hpp"

#include "proto/ECS/components/script_component.pb.h"

namespace astre::ecs::system
{
    class ScriptSystem : public System<proto::ecs::ScriptComponent>
    {
    public:
        using Reads = std::tuple<proto::ecs::TransformComponent, proto::ecs::InputComponent, proto::ecs::CameraComponent>;
        using Writes = std::tuple<proto::ecs::TransformComponent, proto::ecs::CameraComponent>;

        ScriptSystem(script::ScriptRuntime & runtime, Registry & registry);
        ~ScriptSystem() = default;

        inline ScriptSystem(ScriptSystem && other)
            : System(std::move(other)), _runtime(other._runtime)
        {}

        ScriptSystem & operator=(ScriptSystem && other) = delete;

        ScriptSystem(const ScriptSystem &) = delete;
        ScriptSystem & operator=(const ScriptSystem &) = delete;

        void run(float dt);

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }

    private:
        script::ScriptRuntime & _runtime;
    };
}