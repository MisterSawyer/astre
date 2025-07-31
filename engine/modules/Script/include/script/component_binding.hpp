#pragma once

#include <sol/sol.hpp>

#include "math/math.hpp"
#include "input/input.hpp"

#include "generated/ECS/proto/components/transform_component.pb.h"
#include "generated/ECS/proto/components/visual_component.pb.h"
#include "generated/ECS/proto/components/camera_component.pb.h"
#include "generated/ECS/proto/components/light_component.pb.h"
#include "generated/ECS/proto/components/input_component.pb.h"

namespace astre::script
{
    template<class ComponentType>
    struct LuaBinding;


    template<>
    struct LuaBinding<ecs::TransformComponent>
    {
        using bind_type = LuaBinding<ecs::TransformComponent>;

        ecs::TransformComponent & ref;

        inline float get_x() const { return ref.position().x(); }
        inline void set_x(float x) { ref.mutable_position()->set_x(x); }

        inline float get_y() const { return ref.position().y(); }
        inline void set_y(float y) { ref.mutable_position()->set_y(y); }

        inline float get_z() const { return ref.position().z(); }
        inline void set_z(float z) { ref.mutable_position()->set_z(z);}

        inline void set_position(float x, float y, float z) 
        { 
            ref.mutable_position()->set_x(x);
            ref.mutable_position()->set_y(y);
            ref.mutable_position()->set_z(z);
        }

        inline void set_position(const math::Vec3 & position) 
        { 
            ref.mutable_position()->CopyFrom(math::serialize(position));
        }

        inline math::Vec3 get_position() const 
        {
            return math::deserialize(ref.position());
        }

        inline void set_rotation(float w, float x, float y, float z) 
        { 
            ref.mutable_rotation()->set_w(w);
            ref.mutable_rotation()->set_x(x);
            ref.mutable_rotation()->set_y(y);
            ref.mutable_rotation()->set_z(z);
        }

        inline math::Quat get_rotation() const 
        {
            return math::deserialize(ref.rotation());
        }

        inline void set_rotation(const math::Quat & quat) 
        { 
            ref.mutable_rotation()->CopyFrom(math::serialize(quat));
        }

        inline void set_scale(float x, float y, float z) 
        { 
            ref.mutable_scale()->set_x(x);
            ref.mutable_scale()->set_y(y);
            ref.mutable_scale()->set_z(z);
        }

        inline void set_scale(const math::Vec3 & scale) 
        { 
            ref.mutable_scale()->CopyFrom(math::serialize(scale));
        }

        inline math::Vec3 get_scale() const 
        {
            return math::deserialize(ref.scale());
        }

        inline math::Vec3 get_forward() const
        {
            return math::deserialize(ref.forward());
        }

        inline math::Vec3 get_up() const
        {
            return math::deserialize(ref.up());
        }

        inline math::Vec3 get_right() const
        {
            return math::deserialize(ref.right());
        }

        inline void set_forward(const math::Vec3 & forward) 
        { 
            ref.mutable_forward()->CopyFrom(math::serialize(forward));
        }

        inline void set_up(const math::Vec3 & up) 
        { 
            ref.mutable_up()->CopyFrom(math::serialize(up));
        }

        inline void set_right(const math::Vec3 & right) 
        { 
            ref.mutable_right()->CopyFrom(math::serialize(right));
        }


        inline static const auto BINDINGS = std::make_tuple(
            "get_x", &bind_type::get_x,
            "set_x", &bind_type::set_x,

            "get_y", &bind_type::get_y,
            "set_y", &bind_type::set_y,

            "get_z", &bind_type::get_z,
            "set_z", &bind_type::set_z,

            "set_position", sol::overload(
                static_cast<void(bind_type::*)(float, float, float)>(&bind_type::set_position),
                static_cast<void(bind_type::*)(const math::Vec3&)>(&bind_type::set_position)
            ),
            
            "get_position", &bind_type::get_position,

            "set_rotation", sol::overload(
                static_cast<void(bind_type::*)(float, float, float, float)>(&bind_type::set_rotation),
                static_cast<void(bind_type::*)(const math::Quat&)>(&bind_type::set_rotation)
            ),
            
            "get_rotation", &bind_type::get_rotation,

            "set_scale", sol::overload(
                static_cast<void(bind_type::*)(float, float, float)>(&bind_type::set_scale),
                static_cast<void(bind_type::*)(const math::Vec3&)>(&bind_type::set_scale)
            ),

            "get_scale", &bind_type::get_scale,

            "get_forward", &bind_type::get_forward,
            "get_up", &bind_type::get_up,
            "get_right", &bind_type::get_right,

            "set_forward", &bind_type::set_forward,
            "set_up", &bind_type::set_up,
            "set_right", &bind_type::set_right
        );
    };

    template<>
    struct LuaBinding<ecs::VisualComponent>
    {
        using bind_type = LuaBinding<ecs::VisualComponent>;

        ecs::VisualComponent & ref;

       inline bool is_visible() const { return ref.visible(); }
       inline void set_visible(bool visible) { ref.set_visible(visible); }

       inline std::string get_vertex_buffer_name() const { return ref.vertex_buffer_name(); }
       inline void set_vertex_buffer_name(std::string name) { ref.set_vertex_buffer_name(name); }

       inline std::string get_shader_name() const { return ref.shader_name(); }
       inline void set_shader_name(std::string name) { ref.set_shader_name(name); }

       inline astre::math::Vec4 get_color() const { return math::deserialize(ref.color()); }
       inline void set_color(astre::math::Vec4 color) { ref.mutable_color()->CopyFrom(math::serialize(color)); }


        inline static constexpr auto BINDINGS = std::make_tuple(
            "is_visible", &bind_type::is_visible,
            "set_visible", &bind_type::set_visible,

            "get_vertex_buffer_name", &bind_type::get_vertex_buffer_name,
            "set_vertex_buffer_name", &bind_type::set_vertex_buffer_name,

            "get_shader_name", &bind_type::get_shader_name,
            "set_shader_name", &bind_type::set_shader_name,

            "get_color", &bind_type::get_color,
            "set_color", &bind_type::set_color
        );
    };

    template<>
    struct LuaBinding<ecs::CameraComponent>
    {
        using bind_type = LuaBinding<ecs::CameraComponent>;

        ecs::CameraComponent & ref;

        inline float get_fov() const { return ref.fov(); }
        inline void set_fov(float fov) { ref.set_fov(fov); }

        inline float get_near_plane() const { return ref.near_plane(); }
        inline void set_near_plane(float near_plane) { ref.set_near_plane(near_plane); }

        inline float get_far_plane() const { return ref.far_plane(); }
        inline void set_far_plane(float far_plane) { ref.set_far_plane(far_plane); }

        inline float get_aspect() const { return ref.aspect(); }
        inline void set_aspect(float aspect) { ref.set_aspect(aspect); }

        inline static constexpr auto BINDINGS = std::make_tuple(
            "get_fov", &bind_type::get_fov,
            "set_fov", &bind_type::set_fov,

            "get_near_plane", &bind_type::get_near_plane,
            "set_near_plane", &bind_type::set_near_plane,

            "get_far_plane", &bind_type::get_far_plane,
            "set_far_plane", &bind_type::set_far_plane,

            "get_aspect", &bind_type::get_aspect,
            "set_aspect", &bind_type::set_aspect
        );
    };

    template<>
    struct LuaBinding<ecs::LightComponent>
    {
        using bind_type = LuaBinding<ecs::LightComponent>;


        ecs::LightComponent & ref;

/*
    LightType type = 1;
    
    bool cast_shadows = 2;

    // Color and intensity
    optional float color_r = 3;
    optional float color_g = 4;
    optional float color_b = 5;
    optional float intensity = 6;

    // Attenuation (used by point/spot lights)
    optional float constant = 7;
    optional float linear = 8;
    optional float quadratic = 9;

    // Spot-specific cutoff angles (cosine values)
    optional float inner_cutoff = 10;
    optional float outer_cutoff = 11;
*/


        inline void f(){}

        static constexpr auto BINDINGS = std::make_tuple(
        "f", &bind_type::f
        );
    };

    template<>
    struct LuaBinding<ecs::InputComponent>
    {
        using bind_type = LuaBinding<ecs::InputComponent>;


        ecs::InputComponent & ref;

        inline float get_mouse_x() const { return ref.mouse().x(); }
        inline float get_mouse_y() const { return ref.mouse().y(); }

        inline float get_mouse_dx() const { return ref.mouse().dx(); }
        inline float get_mouse_dy() const { return ref.mouse().dy(); }

        inline bool is_pressed(const std::string& key) const { return input::isInputPresent(input::keyToInputCode(key), ref.pressed()); }
        inline bool just_pressed(const std::string& key) const { return input::isInputPresent(input::keyToInputCode(key), ref.just_pressed()); }
        inline bool just_released(const std::string& key) const { return input::isInputPresent(input::keyToInputCode(key), ref.just_released()); }

        inline static constexpr auto BINDINGS = std::make_tuple(
            "get_mouse_x", &bind_type::get_mouse_x,
            "get_mouse_y", &bind_type::get_mouse_y,
            "get_mouse_dx", &bind_type::get_mouse_dx,
            "get_mouse_dy", &bind_type::get_mouse_dy,

            "is_pressed", &bind_type::is_pressed,
            "just_pressed", &bind_type::just_pressed,
            "just_released", &bind_type::just_released
        );
    };
}