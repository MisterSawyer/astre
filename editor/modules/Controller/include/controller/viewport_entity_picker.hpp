#pragma once 

#include <utility>

#include <spdlog/spdlog.h>
#include <imgui.h>

#include "math/math.hpp"
#include "render/render.hpp"
#include "input/input.hpp"
#include "ecs/ecs.hpp"
#include "pipeline/pipeline.hpp"

#include "model/chunk_entities_registry.hpp"

#include "selection_controller.hpp"

namespace astre::editor::controller
{
    class ViewportEntityPicker
    {
    public:
        ViewportEntityPicker(
            render::IRenderer & renderer,
            const input::InputService & input,
            const pipeline::PickingResources & picking_resources,
            const model::ChunkEntityRegistry & chunk_entities_registry)
        :   _renderer(renderer),
            _input(input),
            _picking_resources(picking_resources),
            _chunk_entities_registry(chunk_entities_registry)
        {}

        ~ViewportEntityPicker() = default;

        void setHovered(bool hovered) noexcept { _vp_hovered = hovered; }
        [[nodisard]] bool isHovered() const noexcept { return _vp_hovered; }

        // Viewport info (provided by ViewportPanel each frame)
        void setViewportRect(math::Vec2 content_top_left, math::Vec2 size) noexcept {
            _vp_pos = content_top_left;
            _vp_size = size;
        }

        asio::awaitable<void> updateSelectedEntity(SelectionController & selection_controller)
        {
            if(_vp_hovered && _input.isKeyJustPressed(input::InputCode::MOUSE_LEFT))
            {
                auto relative_mouse_pos = _input.getMousePosition() - _vp_pos;

                relative_mouse_pos.x = std::clamp(relative_mouse_pos.x / _vp_size.x, 0.0f, 0.99f); // [0,1)
                relative_mouse_pos.y = std::clamp(relative_mouse_pos.y / _vp_size.y, 0.0f, 0.99f); // [0,1)

                int x = relative_mouse_pos.x * _picking_resources.size.first;
                int y = (1.0f - relative_mouse_pos.y) * _picking_resources.size.second;

                auto selected_id_res = co_await _renderer.readPixelUint64(_picking_resources.fbo, 0, x, y);
                if(selected_id_res == std::nullopt) co_return;
                
                // update selected entity in selection controller
                // we need to map entity id obtained from texture to chunk id and EntityDefinition
                // to do that we need mapping 
                for(const auto & [chunk_id, entities] : _chunk_entities_registry.mapping)
                {
                    if(entities.contains(*selected_id_res))
                    {
                        selection_controller.setSelection(chunk_id, entities.at(*selected_id_res));
                        co_return;
                    }
                }
            }

            co_return;
        } 

        private:
            render::IRenderer & _renderer;
            const input::InputService & _input;

            const pipeline::PickingResources & _picking_resources;
            const model::ChunkEntityRegistry & _chunk_entities_registry;

            math::Vec2 _vp_pos{0,0};
            math::Vec2 _vp_size{0,0};
            bool _vp_hovered{false};
            bool _captured{false};
    };
}