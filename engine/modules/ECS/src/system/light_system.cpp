#include "ecs/system/transform_system.hpp"

#include "ecs/system/light_system.hpp"

#include <spdlog/spdlog.h>

namespace astre::ecs::system
{
    static proto::ecs::LightType _getLightType(const render::GPULight & light)
    {
        if(light.direction.w == static_cast<float>(proto::ecs::LightType::DIRECTIONAL)) return proto::ecs::LightType::DIRECTIONAL;
        if(light.direction.w == static_cast<float>(proto::ecs::LightType::POINT)) return proto::ecs::LightType::POINT;
        if(light.direction.w == static_cast<float>(proto::ecs::LightType::SPOT)) return proto::ecs::LightType::SPOT;
        return proto::ecs::LightType::UNKNOWN_LightType;
    }


    LightSystem::LightSystem(Registry & registry)
        :   System(registry)
    {
    }

    asio::awaitable<void> LightSystem::run(float dt, render::Frame & frame)
    {
        frame.gpu_lights.clear();

        math::Vec3 position;
        math::Vec3 direction;

        render::GPULight gpu_light{};

        // collect lights
        getRegistry().runOnAllWithComponents<proto::ecs::TransformComponent, proto::ecs::LightComponent>(
            [&](const Entity e, const proto::ecs::TransformComponent & transform_component, const proto::ecs::LightComponent & light_component)
            {
                if(frame.gpu_lights.size() >= MAX_LIGHTS) return;

                position = math::deserialize(transform_component.position());
                direction = math::deserialize(transform_component.forward());

                // Move the light slightly forward along its direction
                // This prevents self-shadowing collapse due to zero distance between camera and light projection centers
                gpu_light.position = math::Vec4(
                    position.x + (direction.x * 0.05f),
                    position.y + (direction.y * 0.05f),
                    position.z + (direction.z * 0.05f),
                    1.0f
                );


                // w component is used to determine the type of light
                gpu_light.direction = glm::vec4(
                    direction.x,
                    direction.y,
                    direction.z, 
                    static_cast<float>(light_component.type())
                );

                gpu_light.color = math::deserialize(light_component.color());

                gpu_light.attenuation = math::Vec4(
                    light_component.constant(),
                    light_component.linear(),
                    light_component.quadratic(),
                    0.0f
                );

                gpu_light.cutoff = math::Vec2(
                    light_component.inner_cutoff(),
                    light_component.outer_cutoff()
                );

                gpu_light.castShadows.x = static_cast<uint32_t>(light_component.cast_shadows());

                frame.gpu_lights[e] = (std::move(gpu_light));

                assert(frame.gpu_lights.size() <= MAX_LIGHTS);
            }
        );

        { // collect shadow casters
            std::size_t shadow_caster_id = 0;
            for (auto & [entity, shadow_caster] : frame.gpu_lights)
            {
                if(shadow_caster_id >= MAX_SHADOW_CASTERS) break;

                // check if this light casts shadows
                if(shadow_caster.castShadows.x == 0) continue;

                // store shadow caster id 
                shadow_caster.castShadows.y = static_cast<float>(shadow_caster_id);

                shadow_caster_id++;

                assert(shadow_caster_id <= MAX_SHADOW_CASTERS);
            }
            // store shadow casters count
            frame.shadow_casters_count = shadow_caster_id;
        }

        co_return;
    }


    std::vector<math::Mat4> calculateLightSpaceMatrices(const render::Frame & interpolated_frame)
    {
        std::vector<math::Mat4> light_space_matrices;
        light_space_matrices.resize(interpolated_frame.shadow_casters_count);

        for (auto& [id, light] : interpolated_frame.gpu_lights) 
        {
            // check if this light casts shadows
            if (light.castShadows.x == 0) continue;

            // obtain shadow caster id
            std::size_t shadow_caster_id = static_cast<std::size_t>(light.castShadows.y);
            if (shadow_caster_id >= interpolated_frame.shadow_casters_count) continue;

            math::Vec3 pos(light.position);
            math::Vec3 dir(light.direction);

            math::Mat4 view = glm::lookAt(
                pos,
            pos + math::normalize(dir),
            ecs::system::TransformSystem::BASE_UP_DIRECTION);

            math::Mat4 proj;
            switch(_getLightType(light))
            {
                case proto::ecs::LightType::DIRECTIONAL:
                {
                    proj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 100.0f);
                    break;
                }

                case proto::ecs::LightType::SPOT: 
                {
                    float outer = glm::clamp(light.cutoff.y, -1.0f, 1.0f);
                    float fov = glm::degrees(2.0f * acos(glm::clamp(outer, -0.999f, 0.999f)));
                    fov = glm::clamp(fov, 5.0f, 179.0f);
                    proj = glm::perspective(glm::radians(fov), 1.7f, 0.1f, 1024.0f);
                    break;
                }

                case proto::ecs::LightType::POINT :
                {
                    proj = math::perspective(math::radians(90.0f), 1.0f, 0.1f, 100.0f);
                    break;
                }

                default:
                    continue;
            }
            
            assert(shadow_caster_id < light_space_matrices.size());
            light_space_matrices.at(shadow_caster_id) = (proj * view);
        }

        return light_space_matrices;
    }

}