#include <stdexcept>

#include <spdlog/spdlog.h>

#include "loader/entity_serializer.hpp"

namespace astre::loader
{
    asio::awaitable<proto::ecs::EntityDefinition> EntitySerializer::serializeEntity(const ecs::Entity entity, const ecs::Registry & registry) const
    {
        proto::ecs::EntityDefinition entity_def;
        auto name_res = co_await registry.getName(entity);
        if (name_res.has_value() == false)
        {
            spdlog::error("Entity {} has no name", entity);
            throw std::runtime_error("Entity has no name");
        }

        entity_def.set_id(entity);
        entity_def.set_name(name_res.value());

        registry.runOnSingleWithComponents<proto::ecs::TransformComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::TransformComponent & component)
            {
                entity_def.mutable_transform()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::VisualComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::VisualComponent & component)
            {
                entity_def.mutable_visual()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::InputComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::InputComponent & component)
            {
                entity_def.mutable_input()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::HealthComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::HealthComponent & component)
            {
                entity_def.mutable_health()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::CameraComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::CameraComponent & component)
            {
                entity_def.mutable_camera()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::TerrainComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::TerrainComponent & component)
            {
                entity_def.mutable_terrain()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::LightComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::LightComponent & component)
            {
                entity_def.mutable_light()->CopyFrom(component);
            });

        registry.runOnSingleWithComponents<proto::ecs::ScriptComponent>(entity,
            [&entity_def](const ecs::Entity, const proto::ecs::ScriptComponent & component)
            {
                entity_def.mutable_script()->CopyFrom(component);
            });

        co_return entity_def;
    }
}
