#pragma once

#include <optional>

#include "render/render.hpp"

#include "ecs/system/system.hpp"

#include "proto/ECS/components/camera_component.pb.h"

namespace astre::ecs::system
{
    class CameraSystem : public System<proto::ecs::CameraComponent>
    {
    public:
        using Reads = std::tuple<proto::ecs::TransformComponent>;
        using Writes = std::tuple<proto::ecs::CameraComponent>;

        CameraSystem(Registry & registry);

        inline CameraSystem(CameraSystem && other)
            : System(std::move(other)), active_camera_entity(std::nullopt)
        {}

        CameraSystem & operator=(CameraSystem && other) = delete;

        CameraSystem(const CameraSystem &) = delete;
        CameraSystem & operator=(const CameraSystem &) = delete;

        ~CameraSystem() = default;

        void run(float dt, render::Frame & frame);
        std::optional<math::Vec3> getActiveCameraPosition() const;

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }

        void setActiveCameraEntity(const ecs::Entity & entity) { active_camera_entity = entity; }
        void deactivateCameraEntity() { active_camera_entity = std::nullopt; }

    private:
        std::optional<ecs::Entity> active_camera_entity;
    };
}
