#include <spdlog/spdlog.h>

#include "ecs/system/input_system.hpp"

namespace astre::ecs::system
{
    InputSystem::InputSystem(input::InputService & input_service, Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        : System(registry, execution_context), _input_service(input_service)
    {}

    asio::awaitable<void> InputSystem::run()
    {
        google::protobuf::RepeatedField<int> serialized_pressed;
        google::protobuf::RepeatedField<int> serialized_just_pressed;
        google::protobuf::RepeatedField<int> serialized_just_released;

        for(const auto & held : _input_service.getPressed())
        {
            serialized_pressed.Add(held);
        }

        for(const auto & pressed : _input_service.getJustPressed())
        {
            serialized_just_pressed.Add(pressed);
        }

        for(const auto & released : _input_service.getJustReleased())
        {
            serialized_just_released.Add(released);
        }

        co_await getRegistry().runOnAllWithComponents<InputComponent>(
            [&](const Entity e, InputComponent & input_component) -> asio::awaitable<void> 
            { 
                input_component.mutable_pressed()->Clear();
                input_component.mutable_just_pressed()->Clear();
                input_component.mutable_just_released()->Clear();
                
                input_component.mutable_pressed()->CopyFrom(serialized_pressed);
                input_component.mutable_just_pressed()->CopyFrom(serialized_just_pressed);
                input_component.mutable_just_released()->CopyFrom(serialized_just_released);

                co_return; 
            }
        );
        co_return;
    } 

}