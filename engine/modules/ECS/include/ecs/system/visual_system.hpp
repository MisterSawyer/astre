#pragma once

#include <optional>
#include <vector>

#include "render/render.hpp"
#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/visual_component.pb.h"

namespace astre::ecs::system
{
    class VisualSystem : public System<VisualComponent>
    {
    public:
        using Reads = std::tuple<TransformComponent, CameraComponent>;
        using Writes = std::tuple<VisualComponent>;

        VisualSystem(const render::IRenderer & renderer, Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

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