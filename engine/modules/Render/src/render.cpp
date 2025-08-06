#include <spdlog/spdlog.h>

#include "render/render.hpp"
#include "formatter/math_format.hpp"

namespace astre::render
{
    Frame interpolateFrame(const Frame& a, const Frame& b, float alpha)
    {     
        alpha = std::clamp(alpha, 0.0f, 1.0f);

        Frame result;

        result.sim_time = b.sim_time;

        result.camera_position = math::mix(a.camera_position, b.camera_position, alpha);

        // interpolate mat4x4
        result.view_matrix = math::interpolate(a.view_matrix, b.view_matrix, alpha);
        result.proj_matrix = math::interpolate(a.proj_matrix, b.proj_matrix, alpha);

        result.render_proxies = b.render_proxies;
        math::Vec3 interpolated_pos;
        math::Quat interpolated_rot;
        math::Vec3 interpolated_scale;
        for(auto & [id, proxy] : result.render_proxies)
        {
            if(a.render_proxies.find(id) == a.render_proxies.end())
                continue;
            
            interpolated_pos = math::mix(a.render_proxies.at(id).position, b.render_proxies.at(id).position, alpha);
            interpolated_rot = math::slerp(a.render_proxies.at(id).rotation, b.render_proxies.at(id).rotation, alpha);
            interpolated_scale = math::mix(a.render_proxies.at(id).scale, b.render_proxies.at(id).scale, alpha);

            proxy.inputs.in_mat4["uModel"] =    
                math::translate(glm::mat4(1.0f), interpolated_pos) *
                math::toMat4(interpolated_rot) *
                math::scale(glm::mat4(1.0f), interpolated_scale);
        }


        result.light_ssbo = a.light_ssbo;
        result.shadow_casters_count = a.shadow_casters_count;

        const std::size_t light_count = std::min(a.gpu_lights.size(), b.gpu_lights.size());
        result.gpu_lights.resize(light_count);
        for (std::size_t i = 0; i < light_count; ++i)
        {
            result.gpu_lights[i].position    = math::mix(a.gpu_lights[i].position,    b.gpu_lights[i].position,    alpha);
            result.gpu_lights[i].color       = math::mix(a.gpu_lights[i].color,       b.gpu_lights[i].color,       alpha);
            result.gpu_lights[i].attenuation = math::mix(a.gpu_lights[i].attenuation, b.gpu_lights[i].attenuation, alpha);
            result.gpu_lights[i].cutoff      = math::mix(a.gpu_lights[i].cutoff,      b.gpu_lights[i].cutoff,      alpha);
            result.gpu_lights[i].castShadows = math::mix(a.gpu_lights[i].castShadows, b.gpu_lights[i].castShadows, alpha);
            result.gpu_lights[i].direction   = math::mix(a.gpu_lights[i].direction,   b.gpu_lights[i].direction,   alpha);
        }

        const std::size_t matrix_count = std::min(a.light_space_matrices.size(), b.light_space_matrices.size());
        result.light_space_matrices.resize(matrix_count);
        for (std::size_t i = 0; i < matrix_count; ++i)
        {
            result.light_space_matrices[i] = math::interpolate(a.light_space_matrices[i], b.light_space_matrices[i], alpha);
        }

        return result;
    }
}