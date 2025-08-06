#include "ecs/system/script_system.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"

#include <spdlog/spdlog.h>

namespace astre::ecs::system
{
    ScriptSystem::ScriptSystem(script::ScriptRuntime & runtime, Registry & registry)
        : System(registry),
        _runtime(runtime)
    {}


    asio::awaitable<void> ScriptSystem::run(float dt)
    {
        auto cs = co_await asio::this_coro::cancellation_state;
        assert(cs.slot().is_connected() && "ScriptSystem run: cancellation_state is not connected");

        spdlog::debug("Running ScriptSystem");


        if(async::isCancelled(cs)) co_return;

        if(async::isCancelled(cs)) co_return;
        co_await getRegistry().runOnAllWithComponents<ScriptComponent>(
            [&](const Entity e, const ScriptComponent & script_component) ->asio::awaitable<void>
            {
                if(_runtime.scriptLoaded(script_component.name()) == false)
                {
                    //spdlog::error("Script {} not loaded", script_component.name());
                    co_return;
                }

                auto & sandbox = _runtime.getEnviroment(e);

                sandbox["entity"] = e;
                sandbox["dt"] = dt;

                if(async::isCancelled(cs)) co_return;
                co_await getRegistry().runOnSingleWithComponents<ecs::TransformComponent>(e,
                [&](const Entity e, ecs::TransformComponent & transform_component) -> asio::awaitable<void>
                {
                    sandbox.set("transform_component", script::LuaBinding<ecs::TransformComponent>(transform_component));
                    co_return;
                });

                if(async::isCancelled(cs)) co_return;
                co_await getRegistry().runOnSingleWithComponents<ecs::InputComponent>(e,
                [&](const Entity e, ecs::InputComponent & input_component) -> asio::awaitable<void>
                {
                    sandbox.set("input_component", script::LuaBinding<ecs::InputComponent>(input_component));
                    co_return;
                });


                if(async::isCancelled(cs)) co_return;                
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

        co_return;
    }
}