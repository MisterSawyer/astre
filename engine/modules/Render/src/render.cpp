#include <spdlog/spdlog.h>

#include "render/render.hpp"

namespace astre::render
{
    static RenderProxy _interpolate(const RenderProxy & a, const RenderProxy & b, float t)
    {
        RenderProxy result;

        result.vertex_buffer = t <= 0.5 ? a.vertex_buffer : b.vertex_buffer;
        result.shader = t <= 0.5 ? a.shader : b.shader;

        auto& ra = a.inputs;
        auto& rb = b.inputs;
        auto& r  = result.inputs;

        // in_int
        for (const auto& [key, va] : ra.in_int)
        {
            if (auto it = rb.in_int.find(key); it != rb.in_int.end())
                r.in_int[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_int[key] = va;
        }

        // in_uint
        for (const auto& [key, va] : ra.in_uint)
        {
            if (auto it = rb.in_int.find(key); it != rb.in_int.end())
                r.in_int[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_int[key] = va;
        }

        // in_float
        for (const auto& [key, va] : ra.in_float)
        {
            if (auto it = rb.in_float.find(key); it != rb.in_float.end())
                r.in_float[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_float[key] = va;
        }

        // in_vec2
        for (const auto& [key, va] : ra.in_vec2)
        {
            if (auto it = rb.in_vec2.find(key); it != rb.in_vec2.end())
                r.in_vec2[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_vec2[key] = va;
        }

        // in_vec3
        for (const auto& [key, va] : ra.in_vec3)
        {
            if (auto it = rb.in_vec3.find(key); it != rb.in_vec3.end())
                r.in_vec3[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_vec3[key] = va;
        }

        // in_vec4
        for (const auto& [key, va] : ra.in_vec4)
        {
            if (auto it = rb.in_vec4.find(key); it != rb.in_vec4.end())
                r.in_vec4[key] = va * (1.0f - t) + it->second * t;
            else
                r.in_vec4[key] = va;
        }

        // in_mat4
        // for (const auto& [key, va] : ra.in_mat4)
        // {
        //     if (auto it = rb.in_mat4.find(key); it != rb.in_mat4.end())
        //         r.in_mat4[key] = math::lerp(va, it->second, t);  // implement element-wise or define lerp
        //     else
        //         r.in_mat4[key] = va;
        // }

        // Non-interpolatable or statically chosen fields
        r.in_bool            = t <= 0.5 ? ra.in_bool : rb.in_bool;
        r.in_mat2            = ra.in_mat2;
        r.in_mat3            = ra.in_mat3;
        r.in_mat4            = ra.in_mat4;
        r.in_mat4_array      = ra.in_mat4_array;
        r.in_samplers        = t <= 0.5 ? ra.in_samplers : rb.in_samplers;
        r.in_samplers_array  = t <= 0.5 ? ra.in_samplers_array : rb.in_samplers_array;
        r.storage_buffers    = t <= 0.5 ? ra.storage_buffers : rb.storage_buffers;

        return result;
    }

    asio::awaitable<void> renderInterpolated(        
        const render::Frame & N2,
        const render::Frame & N1,
        float alpha,
        IRenderer & renderer,
        const render::RenderOptions options,
        const std::optional<std::size_t> fbo)
    {
        if(renderer.good() == false) co_return;

        render::Frame interpolated_frame = N2;
        /*
        if(alpha <= 0.5f)
        {
            // N2 is primary
            for(const auto & proxies : N2.render_proxies)
            {
                if(N1.render_proxies.contains(proxies.first) == false)
                {
                    // in N1 there is no Render proxy matching N2 proxy
                    // we just set to N2
                    interpolated_frame.render_proxies[proxies.first] = proxies.second;
                }
                else
                {
                    // interpolate between the two
                    interpolated_frame.render_proxies[proxies.first]    
                        = _interpolate(proxies.second, N1.render_proxies.at(proxies.first), alpha);
                }
            }
        }
        else
        {
            // N1 is primary
            for(const auto & proxies : N1.render_proxies)
            {
                if(N2.render_proxies.contains(proxies.first) == false)
                {
                    // in N2 there is no Render proxy matching N1 proxy
                    // we just set to N1
                    interpolated_frame.render_proxies[proxies.first] = proxies.second;
                }
                else
                {
                    // interpolate between the two
                    interpolated_frame.render_proxies[proxies.first]    
                        = _interpolate(proxies.second, N1.render_proxies.at(proxies.first), alpha);
                }
            }
        }
        */
        // render interpolated frame
        for(const auto & [_, proxy] : interpolated_frame.render_proxies)
        {
            co_await renderer.render( 
                proxy.vertex_buffer,
                proxy.shader,
                std::move(proxy.inputs),
                options,
                fbo);
        }

        co_return;
    }



}