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

        CameraSystem(std::string active_camera_entity_name, Registry & registry);

        inline CameraSystem(CameraSystem && other)
            : System(std::move(other)), _active_camera_entity_name(std::move(other._active_camera_entity_name))
        {}

        CameraSystem & operator=(CameraSystem && other) = delete;

        CameraSystem(const CameraSystem &) = delete;
        CameraSystem & operator=(const CameraSystem &) = delete;

        ~CameraSystem() = default;


        asio::awaitable<void> run(float dt, render::Frame & frame);

        std::vector<std::type_index> getReads() const override {
            return expand<Reads>();
        }
        
        std::vector<std::type_index> getWrites() const override {
            return expand<Writes>();
        }

        void setActiveCameraEntityName(const std::string & entity_name) { _active_camera_entity_name = entity_name; }

    private:
        std::string _active_camera_entity_name;
    };
}