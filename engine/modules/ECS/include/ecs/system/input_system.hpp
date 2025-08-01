#pragma once

#include "input/input.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/input_component.pb.h"

namespace astre::ecs::system
{
    class InputSystem : public System<InputComponent>
    {
    public:
        InputSystem(input::InputService & input_service, Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        inline InputSystem(InputSystem && other)
            : System(std::move(other)), _input_service(other._input_service)
        {}

        InputSystem & operator=(InputSystem && other) = delete;

        InputSystem(const InputSystem &) = delete;
        InputSystem & operator=(const InputSystem &) = delete;

        ~InputSystem() = default;

        asio::awaitable<void> run();

    private:    
        input::InputService & _input_service;
    };
}