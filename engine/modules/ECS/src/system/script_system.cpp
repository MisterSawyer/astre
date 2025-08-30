#include "ecs/system/script_system.hpp"

#include "proto/ECS/components/transform_component.pb.h"

#include <spdlog/spdlog.h>

namespace astre::ecs::system
{
    ScriptSystem::ScriptSystem(script::ScriptRuntime & runtime, Registry & registry)
        : System(registry),
        _runtime(runtime)
    {}


    void ScriptSystem::run(float dt)
    {
        getRegistry().runOnAllWithComponents<proto::ecs::ScriptComponent>(
            [&](const Entity e, const proto::ecs::ScriptComponent & script_component)
            {
                if(_runtime.scriptLoaded(script_component.name()) == false)
                {
                    //spdlog::error("Script {} not loaded", script_component.name());
                    return;
                }

                auto & sandbox = _runtime.getEnviroment(e);

                sandbox["entity"] = e;
                sandbox["dt"] = dt;

                getRegistry().runOnSingleWithComponents<proto::ecs::TransformComponent>(e,
                [&](const Entity e, proto::ecs::TransformComponent & transform_component)
                {
                    sandbox.set("transform_component", script::LuaBinding<proto::ecs::TransformComponent>(transform_component));
                });

                getRegistry().runOnSingleWithComponents<proto::ecs::InputComponent>(e,
                [&](const Entity e, proto::ecs::InputComponent & input_component)
                {
                    sandbox.set("input_component", script::LuaBinding<proto::ecs::InputComponent>(input_component));
                });

                auto script = _runtime.getScript(script_component.name());

                try     
                {
                    sandbox.set_on(script);
                    script(); // execute inline
                }
                catch (const std::exception& e) 
                {
                    spdlog::error(std::format("Script execution failed [{}]: {}", script_component.name(), e.what()));
                }
            }
        );
    }
}