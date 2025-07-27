#pragma once

#include "render/render.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/camera_component.pb.h"

namespace astre::ecs::system
{
    class CameraSystem : public System<astre::ecs::CameraComponent>
    {
    public:
        CameraSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        ~CameraSystem() = default;

        asio::awaitable<void> run(render::Frame & frame);

        void setActiveCameraEntityName(const std::string & entity_name) { _active_camera_entity_name = entity_name; }

    private:
        std::string _active_camera_entity_name;
    };
}