#pragma once

#include "script/script.hpp"
#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/script_component.pb.h"

namespace astre::ecs::system
{
    class ScriptSystem : public System<ScriptComponent>
    {
    public:
        ScriptSystem(script::ScriptRuntime & runtime, Registry & registry, astre::process::IProcess::execution_context_type & execution_context);
        ~ScriptSystem() = default;

        inline ScriptSystem(ScriptSystem && other)
            : System(std::move(other)), _runtime(other._runtime)
        {}

        ScriptSystem & operator=(ScriptSystem && other) = delete;

        ScriptSystem(const ScriptSystem &) = delete;
        ScriptSystem & operator=(const ScriptSystem &) = delete;

        asio::awaitable<void> run();

    private:
        script::ScriptRuntime & _runtime;
    };
}