#include <spdlog/spdlog.h>

#include "ecs/system/input_system.hpp"

namespace astre::ecs::system
{
    InputSystem::InputSystem(input::InputService & input_service, Registry & registry)
        : System(registry), _input_service(input_service)
    {}

    asio::awaitable<void> InputSystem::run(float dt)
    {
        google::protobuf::RepeatedField<int> serialized_pressed;
        google::protobuf::RepeatedField<int> serialized_just_pressed;
        google::protobuf::RepeatedField<int> serialized_just_released;

        const auto & mouse_pos = _input_service.getMousePosition();
        const auto & mouse_delta = _input_service.getMouseDelta();

        for(const auto & held : _input_service.getHeld())
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

        getRegistry().runOnAllWithComponents<proto::ecs::InputComponent>(
            [&](const Entity e, proto::ecs::InputComponent & input_component)
            { 
                input_component.mutable_pressed()->Clear();
                input_component.mutable_just_pressed()->Clear();
                input_component.mutable_just_released()->Clear();
                
                input_component.mutable_pressed()->CopyFrom(serialized_pressed);
                input_component.mutable_just_pressed()->CopyFrom(serialized_just_pressed);
                input_component.mutable_just_released()->CopyFrom(serialized_just_released);

                input_component.mutable_mouse()->set_x(mouse_pos.x);
                input_component.mutable_mouse()->set_y(mouse_pos.y);
                input_component.mutable_mouse()->set_dx(mouse_delta.x);
                input_component.mutable_mouse()->set_dy(mouse_delta.y);
            }
        );

        co_return;
    } 

}