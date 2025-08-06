#include <spdlog/spdlog.h>

#include "render/render.hpp"

namespace astre::render
{
    static ShaderInputs _interpolateShaderInputs(const ShaderInputs& a, const ShaderInputs& b, float alpha) {
        ShaderInputs result;

        // Copy non-interpolatable inputs (logic assumes `b` dominates layout)
        result.in_bool = b.in_bool;
        result.in_samplers = b.in_samplers;
        result.in_samplers_array = b.in_samplers_array;
        result.storage_buffers = b.storage_buffers;

        result.in_int = b.in_int; // TODO
        result.in_uint = b.in_uint; //TODO 
        result.in_mat4_array = b.in_mat4_array; //TODO: Interpolate mat4x4 arrays

        // Interpolate float
        for (const auto& [key, valA] : a.in_float) {
            if (auto it = b.in_float.find(key); it != b.in_float.end())
                result.in_float[key] = std::lerp(valA, it->second, alpha);
        }

        // Interpolate Vec2
        for (const auto& [key, valA] : a.in_vec2) {
            if (auto it = b.in_vec2.find(key); it != b.in_vec2.end())
                result.in_vec2[key] = glm::mix(valA, it->second, alpha);
        }

        // Interpolate Vec3
        for (const auto& [key, valA] : a.in_vec3) {
            if (auto it = b.in_vec3.find(key); it != b.in_vec3.end())
                result.in_vec3[key] = glm::mix(valA, it->second, alpha);
        }

        // Interpolate Vec4
        for (const auto& [key, valA] : a.in_vec4) {
            if (auto it = b.in_vec4.find(key); it != b.in_vec4.end())
                result.in_vec4[key] = glm::mix(valA, it->second, alpha);
        }

        result.in_mat2 = b.in_mat2;
        // Interpolate Mat2
        // for (const auto& [key, valA] : a.in_mat2) {
        //     if (auto it = b.in_mat2.find(key); it != b.in_mat2.end())
        //         result.in_mat2[key] = glm::mix(valA, it->second, alpha);
        // }

        result.in_mat3 = b.in_mat3;
        // Interpolate Mat3
        // for (const auto& [key, valA] : a.in_mat3) {
        //     if (auto it = b.in_mat3.find(key); it != b.in_mat3.end())
        //         result.in_mat3[key] = glm::mix(valA, it->second, alpha);
        // }

        // Interpolate Mat4
        for (const auto& [key, valA] : a.in_mat4) {
            if (auto it = b.in_mat4.find(key); it != b.in_mat4.end())
                result.in_mat4[key] = glm::interpolate(valA, it->second, alpha);
        }

        return result;
    }


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
        // for(auto & [id, proxy] : result.render_proxies)
        // {
        //     if(a.render_proxies.find(id) == a.render_proxies.end())
        //         continue;
        //     if(b.render_proxies.find(id) == b.render_proxies.end())
        //         continue;
        //     proxy.inputs = _interpolateShaderInputs(a.render_proxies.at(id).inputs, b.render_proxies.at(id).inputs, alpha);
        // }


        result.light_ssbo = b.light_ssbo;
        result.shadow_casters_count = b.shadow_casters_count;

        const std::size_t light_count = std::min(a.gpu_lights.size(), b.gpu_lights.size());
        result.gpu_lights.resize(light_count);
        for (std::size_t i = 0; i < light_count; ++i)
        {
            result.gpu_lights[i].position    = math::mix(a.gpu_lights[i].position,    b.gpu_lights[i].position,    alpha);
            result.gpu_lights[i].color       = math::mix(a.gpu_lights[i].color,       b.gpu_lights[i].color,       alpha);
            result.gpu_lights[i].attenuation = math::mix(a.gpu_lights[i].attenuation, b.gpu_lights[i].attenuation, alpha);
            result.gpu_lights[i].cutoff      = math::mix(a.gpu_lights[i].cutoff,      b.gpu_lights[i].cutoff,      alpha);
            result.gpu_lights[i].castShadows = math::mix(a.gpu_lights[i].castShadows, b.gpu_lights[i].castShadows, alpha);

            // Direction: slerp between normalized direction vectors
            const math::Vec3 dirA = math::normalize(math::Vec3(a.gpu_lights[i].direction));
            const math::Vec3 dirB = math::normalize(math::Vec3(b.gpu_lights[i].direction));

            //construct quaternions from vector rotation
            math::Quat rotA = math::rotation(math::Vec3(0, 0, -1), dirA);
            math::Quat rotB = math::rotation(math::Vec3(0, 0, -1), dirB);

            //slerp between quaternions
            math::Quat rotInterp = math::slerp(rotA, rotB, alpha);

            //rotate base direction to get interpolated vector
            math::Vec3 dirInterp = math::normalize(rotInterp * math::Vec3(0, 0, -1));

            //store back in Vec4 (preserve w)
            result.gpu_lights[i].direction = math::Vec4(dirInterp, a.gpu_lights[i].direction.w);
        }

        const std::size_t matrix_count = std::min(a.light_space_matrices.size(), b.light_space_matrices.size());
        result.light_space_matrices.resize(matrix_count);
        for (std::size_t i = 0; i < matrix_count; ++i)
        {
            result.light_space_matrices[i] = b.light_space_matrices[i];//math::interpolate(a.light_space_matrices[i], b.light_space_matrices[i], alpha);
        }

        return result;
    }
}