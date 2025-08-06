#include <spdlog/spdlog.h>

#include "ecs/system/transform_system.hpp"
#include "ecs/system/camera_system.hpp"

namespace astre::ecs::system
{
    CameraSystem::CameraSystem(std::string active_camera_entity_name, Registry & registry)
        :   System(registry),
            _active_camera_entity_name(std::move(active_camera_entity_name))
    {}

    asio::awaitable<void> CameraSystem::run(float dt, render::Frame & frame)
    {
        auto cs = co_await asio::this_coro::cancellation_state;
        assert(cs.slot().is_connected() && "CameraSystem run: cancellation_state is not connected");

        spdlog::debug("Running CameraSystem");


        frame.camera_position = math::Vec3(0.0f);
        frame.view_matrix = math::Mat4(1.0f);
        frame.proj_matrix = math::Mat4(1.0f);

        const auto entity = co_await getRegistry().getEntity(_active_camera_entity_name);
        if(!entity)
        {
            spdlog::error("{} entity not loaded in ECS registry", _active_camera_entity_name);
            co_return;
        }

        co_await getRegistry().runOnSingleWithComponents<TransformComponent, CameraComponent>(*entity,
            [&](const Entity e, const TransformComponent & transform_component, const CameraComponent & camera_component) -> asio::awaitable<void>
            {
                frame.camera_position = math::deserialize(transform_component.position());
                frame.view_matrix = math::deserialize(transform_component.transform_matrix());
                frame.proj_matrix = math::perspective(  
                    math::radians(camera_component.fov()),
                    camera_component.aspect(),
                    camera_component.near_plane(),
                    camera_component.far_plane());
                co_return;
            }
        );

        co_return;
    } 

}