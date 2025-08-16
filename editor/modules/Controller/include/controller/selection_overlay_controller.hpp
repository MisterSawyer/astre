#pragma once

#include "render/render.hpp"

namespace astre::editor::controller
{
    class SelectionOverlayController
    {
        public:
            SelectionOverlayController(const render::IRenderer & renderer)
            {
                const std::optional<std::size_t> cube_prefab_res = renderer.getVertexBuffer("cube_prefab");
                if(!cube_prefab_res) throw std::runtime_error("cube prefab not found");
                _cube_prefab = cube_prefab_res.value();

                _selection_render_proxy = render::RenderProxy
                {
                    .visible = true,
                    .phases = render::RenderPhase::Debug,
                    .vertex_buffer = _cube_prefab,
                    // no shader will use debug_overlay by default
                };
                _selection_render_proxy.inputs.in_vec4["uColor"] = math::Vec4(1.0, 0.8, 0.2, 1.0);

                _selection_render_proxy.options = render::RenderOptions
                {
                    .mode = render::RenderMode::Wireframe,
                    .write_depth = false,
                };
            }

            void update(const std::optional<std::pair<astre::world::ChunkID, astre::ecs::EntityDefinition>> & selected_entity_def_res, const render::Frame & render_frame)
            {
                if(selected_entity_def_res == std::nullopt)
                {
                    _visible = false;
                    return;
                }

                const auto & selected_entity_def = *selected_entity_def_res;

                if(selected_entity_def.second.has_transform() == false)
                {
                    _visible = false;
                    return;
                }

                _selection_render_proxy.position = math::deserialize(selected_entity_def.second.transform().position());
                _selection_render_proxy.rotation = math::deserialize(selected_entity_def.second.transform().rotation());
                _selection_render_proxy.scale = math::deserialize(selected_entity_def.second.transform().scale()) * 1.1f;
                     
                _selection_render_proxy.inputs.in_mat4 = {
                    {"uView", render_frame.view_matrix},
                    {"uProjection", render_frame.proj_matrix},
                    {"uModel", 
                        math::translate(glm::mat4(1.0f), _selection_render_proxy.position) *
                        math::toMat4(_selection_render_proxy.rotation) *
                        math::scale(glm::mat4(1.0f), _selection_render_proxy.scale)
                    }
                };

                _visible = true;
            }

            void draw(render::Frame & render_frame)
            {
                if(_visible == false) return;
                // TODO assign free ID
                render_frame.render_proxies[1000] = _selection_render_proxy;
            }


        private:
            bool _visible;
            std::size_t _cube_prefab;
            render::RenderProxy _selection_render_proxy;
    };
}