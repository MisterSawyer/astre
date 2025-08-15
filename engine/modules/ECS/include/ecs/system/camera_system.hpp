#pragma once

#include "render/render.hpp"

#include "ecs/system/system.hpp"

#include "generated/ECS/proto/components/camera_component.pb.h"

namespace astre::ecs::system
{
    class CameraSystem : public System<CameraComponent>
    {
    public:
        using Reads = std::tuple<TransformComponent>;
        using Writes = std::tuple<CameraComponent>;

        CameraSystem(ecs::Entity active_camera_entity, Registry & registry);

        inline CameraSystem(CameraSystem && other)
            : System(std::move(other)), active_camera_entity(std::move(other.active_camera_entity))
        {}

        CameraSystem & operator=(CameraSystem && other) = delete;

        CameraSystem(const CameraSystem &) = delete;
        CameraSystem & operator=(const CameraSystem &) = delete;

        ~CameraSystem() = default;

        void run(float dt, render::Frame & frame);

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }

        void setActiveCameraEntityName(const ecs::Entity & entity) { active_camera_entity = entity; }

    private:
        ecs::Entity active_camera_entity;
    };
}