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
                frame.camera_position = math::deserialize(transform_component.position());
                frame.view_matrix = math::deserialize(transform_component.transform_matrix());
                frame.proj_matrix = math::perspective(  
                    math::radians(camera_component.fov()),
                    camera_component.aspect(),
                    camera_component.near_plane(),
                    camera_component.far_plane());
            }
        );
    } 

}