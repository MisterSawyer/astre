#include <spdlog/spdlog.h>

#include "ecs/system/transform_system.hpp"

namespace astre::ecs::system
{
    TransformSystem::TransformSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        : System(registry, execution_context)
    {}

    asio::awaitable<void> TransformSystem::run()
    {
        co_await getAsyncContext().ensureOnStrand();

        ecs::Registry & registry = getRegistry();

        math::Vec3 pos;
        math::Quat rot;
        math::Vec3 scale;

        math::Vec3 forward;
        math::Vec3 up;
        math::Vec3 right;

        TransformComponent * transform_component = nullptr;

        for (const auto& [entity, mask] : registry.getAllEntities())
        {
            if (mask.test(MASK_BIT) == false) continue;
            
            transform_component = registry.getComponent<TransformComponent>(entity);
            assert(transform_component != nullptr);

            pos = math::deserialize(transform_component->position());
            rot = math::deserialize(transform_component->rotation());
            scale = math::deserialize(transform_component->scale());

            transform_component->mutable_transform_matrix()->CopyFrom(
                math::serialize(
                    math::translate(glm::mat4(1.0f), pos) *
                    math::toMat4(rot) *
                    math::scale(glm::mat4(1.0f), scale)
                )
            );

            forward = math::normalize(rot * BASE_FORWARD_DIRECTION);
            up = math::normalize(rot * BASE_UP_DIRECTION);
            right = math::normalize(math::cross(forward, up));

            transform_component->mutable_forward()->CopyFrom(math::serialize(forward));
            transform_component->mutable_up()->CopyFrom(math::serialize(up));
            transform_component->mutable_right()->CopyFrom(math::serialize(right));
        }

        co_return;
    } 

}