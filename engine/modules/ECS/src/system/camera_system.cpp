#include <spdlog/spdlog.h>

#include "ecs/system/transform_system.hpp"
#include "ecs/system/camera_system.hpp"

namespace astre::ecs::system
{
    CameraSystem::CameraSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        :   System(registry, execution_context)
    {}

    asio::awaitable<void> CameraSystem::run(render::Frame & frame)
    {
        co_await getAsyncContext().ensureOnStrand();
        frame.camera_position = math::Vec3(0.0f);
        frame.view_matrix = math::Mat4(1.0f);
        frame.proj_matrix = math::Mat4(1.0f);

        ecs::Registry & registry = getRegistry();

        const auto entity = registry.getEntity(_active_camera_entity_name);
        if(!entity)
        {
            spdlog::error("{} entity not loaded in ECS registry", _active_camera_entity_name);
            co_return;
        }

        CameraComponent * camera_component = registry.getComponent<CameraComponent>(*entity);
        if(camera_component == nullptr)
        {
            spdlog::error("{} entity does not have CameraComponent attached", _active_camera_entity_name);
            co_return;
        }

        TransformComponent * transform_component = registry.getComponent<TransformComponent>(*entity);
        if(transform_component == nullptr)
        {
            spdlog::error("{} entity does not have TransformComponent attached", _active_camera_entity_name);
            co_return;
        }

        frame.camera_position = math::deserialize(transform_component->position());
        frame.view_matrix = math::deserialize(transform_component->transform_matrix());
        frame.proj_matrix = math::perspective(math::radians(camera_component->fov()), camera_component->aspect(), camera_component->near_plane(), camera_component->far_plane());

        co_return;
    } 

}