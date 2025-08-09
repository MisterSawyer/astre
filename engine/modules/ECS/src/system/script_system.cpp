#include "ecs/system/script_system.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"

#include <spdlog/spdlog.h>

namespace astre::ecs::system
{
    ScriptSystem::ScriptSystem(script::ScriptRuntime & runtime, Registry & registry)
        : System(registry),
        _runtime(runtime)
    {}


    void ScriptSystem::run(float dt)
    {
        getRegistry().runOnAllWithComponents<ScriptComponent>(
            [&](const Entity e, const ScriptComponent & script_component)
            {
                if(_runtime.scriptLoaded(script_component.name()) == false)
                {
                    //spdlog::error("Script {} not loaded", script_component.name());
                    return;
                }

                auto & sandbox = _runtime.getEnviroment(e);

                sandbox["entity"] = e;
                sandbox["dt"] = dt;

                getRegistry().runOnSingleWithComponents<ecs::TransformComponent>(e,
                [&](const Entity e, ecs::TransformComponent & transform_component)
                {
                    sandbox.set("transform_component", script::LuaBinding<ecs::TransformComponent>(transform_component));
                });

                getRegistry().runOnSingleWithComponents<ecs::InputComponent>(e,
                [&](const Entity e, ecs::InputComponent & input_component)
                {
                    sandbox.set("input_component", script::LuaBinding<ecs::InputComponent>(input_component));
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