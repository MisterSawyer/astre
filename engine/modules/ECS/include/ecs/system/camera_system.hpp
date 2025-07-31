#pragma once

#include "render/render.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/camera_component.pb.h"

namespace astre::ecs::system
{
    class CameraSystem : public System<astre::ecs::CameraComponent>
    {
    public:
        CameraSystem(std::string active_camera_entity_name, Registry & registry, astre::process::IProcess::execution_context_type & execution_context);

        inline CameraSystem(CameraSystem && other)
            : System(std::move(other)), _active_camera_entity_name(std::move(other._active_camera_entity_name))
        {}

        CameraSystem & operator=(CameraSystem && other) = delete;

        CameraSystem(const CameraSystem &) = delete;
        CameraSystem & operator=(const CameraSystem &) = delete;

        ~CameraSystem() = default;


        asio::awaitable<void> run(render::Frame & frame);

        void setActiveCameraEntityName(const std::string & entity_name) { _active_camera_entity_name = entity_name; }

    private:
        std::string _active_camera_entity_name;
    };
}