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
        VisualSystem(const render::IRenderer & renderer, Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        ~VisualSystem() = default;
        
        asio::awaitable<void> run(render::Frame & frame);
        
    private:
        const render::IRenderer & _renderer;
    };
}