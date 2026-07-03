#include <spdlog/spdlog.h>

#include "ecs/system/transform_system.hpp"
#include "ecs/system/camera_system.hpp"

namespace astre::ecs::system
{
    CameraSystem::CameraSystem(ecs::Entity active_camera_entity, Registry & registry)
        :   System(registry),
            active_camera_entity(std::move(active_camera_entity))
    {}

    void CameraSystem::run(float dt, render::Frame & frame)
    {
        frame.camera_position = math::Vec3(0.0f);
        frame.view_matrix = math::Mat4(1.0f);
        frame.proj_matrix = math::Mat4(1.0f);

        getRegistry().runOnSingleWithComponents<proto::ecs::TransformComponent, proto::ecs::CameraComponent>(active_camera_entity,
            [&](const Entity e, const proto::ecs::TransformComponent & transform_component, const proto::ecs::CameraComponent & camera_component)
            {
                const math::Vec3 position = math::deserialize(transform_component.position());
                const math::Vec3 forward = math::deserialize(transform_component.forward());
                const math::Vec3 up = math::deserialize(transform_component.up());

                frame.camera_position = position;
                // View matrix is the inverse of the camera's world transform, not the
                // model matrix itself - build it from position/forward/up so movement
                // follows where the camera looks
                frame.view_matrix = math::lookAt(position, position + forward, up);
                frame.proj_matrix = math::perspective(  
                    math::radians(camera_component.fov()),
                    camera_component.aspect(),
                    camera_component.near_plane(),
                    camera_component.far_plane());
            }
        );
    } 

}