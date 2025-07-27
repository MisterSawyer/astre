#include <spdlog/spdlog.h>

#include "ecs/system/visual_system.hpp"
#include "ecs/system/transform_system.hpp"

namespace astre::ecs::system
{
    VisualSystem::VisualSystem(const render::IRenderer & renderer, Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        :   System(registry, execution_context),
            _renderer(renderer)
    {}


    asio::awaitable<void> VisualSystem::run(render::Frame & frame)
    {
        co_await getAsyncContext().ensureOnStrand();
        frame.render_proxies.clear();

        ecs::Registry & registry = getRegistry();

        std::optional<std::size_t> vb_id;
        std::optional<std::size_t> sh_id;
        VisualComponent * visual_component = nullptr;
        TransformComponent * transform_component = nullptr;

        for (const auto& [entity, mask] : registry.getAllEntities())
        {
            if (mask.test(MASK_BIT) == false) continue;
            
            visual_component = registry.getComponent<VisualComponent>(entity);
            assert(visual_component != nullptr);
            vb_id = _renderer.getVertexBuffer(visual_component->vertex_buffer_name());
            sh_id = _renderer.getShader(visual_component->shader_name());

            if (!vb_id || !sh_id)
            {
                // if cannot find vertex buffer or shader, skip
                continue;
            }

            frame.render_proxies[entity].vertex_buffer = *vb_id;
            frame.render_proxies[entity].shader = *sh_id;
            
            // TODO right now every render proxy will copy uView and uProjection
            frame.render_proxies[entity].inputs.in_mat4["uView"] = frame.view_matrix;
            frame.render_proxies[entity].inputs.in_mat4["uProjection"] = frame.proj_matrix;

            frame.render_proxies[entity].inputs.in_bool["useTexture"] = false;

            if(visual_component->has_color())
            {
                math::Vec4 color = math::deserialize(visual_component->color());
                frame.render_proxies[entity].inputs.in_vec4["uColor"] = color;
            }
            else{
                frame.render_proxies[entity].inputs.in_vec4["uColor"] = math::Vec4(1.0f, 0.0f, 1.0f, 1.0f);
            }

            // if also has a transform component
            // update transform matrix
            if(mask.test(TransformSystem::MASK_BIT))
            {
                transform_component = registry.getComponent<TransformComponent>(entity);
                assert(transform_component != nullptr);
                frame.render_proxies[entity].inputs.in_mat4["uModel"] = math::deserialize(transform_component->transform_matrix());
            }
        }

        co_return;
    }     
    
}