#include <spdlog/spdlog.h>

#include "ecs/system/visual_system.hpp"
#include "ecs/system/transform_system.hpp"

namespace astre::ecs::system
{
    VisualSystem::VisualSystem(const render::IRenderer & renderer, Registry & registry)
        :   System(registry),
            _renderer(renderer)
    {}


    asio::awaitable<void> VisualSystem::run(float dt, render::Frame & frame)
    {
        frame.render_proxies.clear();
        
        std::optional<std::size_t> vb_id;
        std::optional<std::size_t> sh_id;
        getRegistry().runOnAllWithComponents<TransformComponent, VisualComponent>(
            [&](const Entity e, const TransformComponent & transform_component, const VisualComponent &visual_component)
            {
                vb_id = _renderer.getVertexBuffer(visual_component.vertex_buffer_name());
                sh_id = _renderer.getShader(visual_component.shader_name());

                if (!vb_id || !sh_id)
                {
                    // if cannot find vertex buffer or shader, skip
                    return;
                }
                
                frame.render_proxies[e].visible = visual_component.visible();

                frame.render_proxies[e].vertex_buffer = *vb_id;
                frame.render_proxies[e].shader = *sh_id;

                frame.render_proxies[e].inputs.in_bool["useTexture"] = false;
                
                if(visual_component.has_color())
                {
                    frame.render_proxies[e].inputs.in_vec4["uColor"] = math::deserialize(visual_component.color());
                }
                else{
                    frame.render_proxies[e].inputs.in_vec4["uColor"] = math::Vec4(1.0f, 0.0f, 1.0f, 1.0f);
                }

                frame.render_proxies[e].inputs.in_mat4["uModel"] = math::deserialize(transform_component.transform_matrix());

                // used for interpolation
                frame.render_proxies[e].position = math::deserialize(transform_component.position());
                frame.render_proxies[e].rotation = math::deserialize(transform_component.rotation());
                frame.render_proxies[e].scale = math::deserialize(transform_component.scale());

                // render during opaque and shadow casting phases
                frame.render_proxies[e].phases =  render::RenderPhase::Opaque | render::RenderPhase::ShadowCaster;
            }

        );

        co_return;
    }     
    
}