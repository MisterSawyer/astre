#pragma once

#include <optional>
#include <vector>

#include "render/render.hpp"
#include "ecs/system/system.hpp"

#include "proto/ECS/components/visual_component.pb.h"

namespace astre::ecs::system
{
    class VisualSystem : public System<proto::ecs::VisualComponent>
    {
    public:
        using Reads = std::tuple<proto::ecs::TransformComponent, proto::ecs::CameraComponent>;
        using Writes = std::tuple<proto::ecs::VisualComponent>;

        VisualSystem(const render::IRenderer & renderer, Registry & registry);

        inline VisualSystem(VisualSystem && other)
            : System(std::move(other)), _renderer(other._renderer)
        {}

        VisualSystem & operator=(VisualSystem && other) = delete;

        VisualSystem(const VisualSystem &) = delete;
        VisualSystem & operator=(const VisualSystem &) = delete;

        ~VisualSystem() = default;
        
        asio::awaitable<void> run(float dt, render::Frame & frame);
        
        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }


    private:
        const render::IRenderer & _renderer;
    };
}