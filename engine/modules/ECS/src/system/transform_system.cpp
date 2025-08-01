#include <spdlog/spdlog.h>

#include "ecs/system/transform_system.hpp"

namespace astre::ecs::system
{
    TransformSystem::TransformSystem(Registry & registry, astre::process::IProcess::execution_context_type & execution_context)
        : System(registry, execution_context)
    {}

    asio::awaitable<void> TransformSystem::run()
    {
        math::Vec3 pos;
        math::Quat rot;
        math::Vec3 scale;

        math::Vec3 forward;
        math::Vec3 up;
        math::Vec3 right;
        
        co_await getAsyncContext().ensureOnStrand();

        co_await getRegistry().runOnAllWithComponents<TransformComponent>(
            [&](const Entity e, TransformComponent & transform_component) ->  asio::awaitable<void>
            {
                if(transform_component.has_position())
                {
                    pos = math::deserialize(transform_component.position());
                }else
                {
                    pos = math::Vec3(0.0f);
                }

                if(transform_component.has_rotation())
                {
                    rot = math::deserialize(transform_component.rotation());
                }
                else
                {
                    // TODO
                    rot = math::Quat(1.0f, 0.0f, 0.0f, 0.0f);
                }

                if(transform_component.has_scale())
                {
                    scale = math::deserialize(transform_component.scale());
                }
                else 
                {
                    scale = math::Vec3(1.0f);
                }

                transform_component.mutable_transform_matrix()->CopyFrom(
                    math::serialize(
                        math::translate(glm::mat4(1.0f), pos) *
                        math::toMat4(rot) *
                        math::scale(glm::mat4(1.0f), scale)
                    )
                );

                forward = math::normalize(rot * BASE_FORWARD_DIRECTION);
                up = math::normalize(rot * BASE_UP_DIRECTION);
                right = math::normalize(math::cross(forward, up));

                transform_component.mutable_forward()->CopyFrom(math::serialize(forward));
                transform_component.mutable_up()->CopyFrom(math::serialize(up));
                transform_component.mutable_right()->CopyFrom(math::serialize(right));

                co_return;
            }
        );

        co_return;
    } 

}