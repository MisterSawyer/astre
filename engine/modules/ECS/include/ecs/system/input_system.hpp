#pragma once

#include "input/input.hpp"

#include "ecs/system/system.hpp"

#include "proto/ECS/components/input_component.pb.h"

namespace astre::ecs::system
{
    class InputSystem : public System<proto::ecs::InputComponent>
    {
    public:
        using Reads = std::tuple<>;
        using Writes = std::tuple<proto::ecs::InputComponent>;

        InputSystem(input::InputService & input_service, Registry & registry);

        inline InputSystem(InputSystem && other)
            : System(std::move(other)), _input_service(other._input_service)
        {}

        InputSystem & operator=(InputSystem && other) = delete;

        InputSystem(const InputSystem &) = delete;
        InputSystem & operator=(const InputSystem &) = delete;

        ~InputSystem() = default;

        asio::awaitable<void> run(float dt);

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }

    private:    
        input::InputService & _input_service;
    };
}