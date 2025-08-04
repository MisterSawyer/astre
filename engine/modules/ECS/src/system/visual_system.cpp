#include <spdlog/spdlog.h>

#include "ecs/system/visual_system.hpp"
#include "ecs/system/transform_system.hpp"

namespace astre::ecs::system
{
    VisualSystem::VisualSystem(const render::IRenderer & renderer, Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        :   System(registry, execution_context),
            _renderer(renderer)
    {}


    asio::awaitable<void> VisualSystem::run(float dt, render::Frame & frame)
    {
        co_await getAsyncContext().ensureOnStrand();

        frame.render_proxies.clear();
        
        std::optional<std::size_t> vb_id;
        std::optional<std::size_t> sh_id;
        co_await getRegistry().runOnAllWithComponents<TransformComponent, VisualComponent>(
            [&](const Entity e, const TransformComponent & transform_component, const VisualComponent &visual_component)  -> asio::awaitable<void>
            {
                vb_id = _renderer.getVertexBuffer(visual_component.vertex_buffer_name());
                sh_id = _renderer.getShader(visual_component.shader_name());

                if (!vb_id || !sh_id)
                {
                    // if cannot find vertex buffer or shader, skip
                    co_return;
                }
                
                frame.render_proxies[e].vertex_buffer = *vb_id;
                frame.render_proxies[e].shader = *sh_id;

                // TODO right now every render proxy will copy uView and uProjection
                frame.render_proxies[e].inputs.in_mat4["uView"] = frame.view_matrix;
                frame.render_proxies[e].inputs.in_mat4["uProjection"] = frame.proj_matrix;

                frame.render_proxies[e].inputs.in_bool["useTexture"] = false;
                
                if(visual_component.has_color())
                {
                    frame.render_proxies[e].inputs.in_vec4["uColor"] = math::deserialize(visual_component.color());
                }
                else{
                    frame.render_proxies[e].inputs.in_vec4["uColor"] = math::Vec4(1.0f, 0.0f, 1.0f, 1.0f);
                }

                frame.render_proxies[e].inputs.in_mat4["uModel"] = math::deserialize(transform_component.transform_matrix());

                co_return;
            }

        );

        co_return;
    }     
    
}