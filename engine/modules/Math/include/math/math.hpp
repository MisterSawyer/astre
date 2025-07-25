#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "generated/Math/proto/math.pb.h"

namespace astre::math
{
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Quat = glm::quat;
    using Mat2 = glm::mat2;
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;

    // Forwarding templated constants
    template<typename T>
    constexpr T two_pi() { return glm::two_pi<T>(); }

    // === Explicit normalize overloads ===
    template<int L, typename T, glm::qualifier Q>
    inline glm::vec<L, T, Q> normalize(const glm::vec<L, T, Q>& v) {return glm::normalize(v);}

    template<typename T, glm::qualifier Q>
    inline glm::qua<T, Q> normalize(const glm::qua<T, Q>& q) {return glm::normalize(q);}

    // === Explicit cross overloads ===
    template<typename T, glm::qualifier Q>
    inline auto cross(const glm::vec<3, T, Q>& x, const glm::vec<3, T, Q>& y) {return glm::cross(x, y);}

    template<typename T, glm::qualifier Q>
    inline auto cross(const glm::vec<2, T, Q>& x, const glm::vec<2, T, Q>& y) {return glm::cross(x, y);}

    template<typename T, glm::qualifier Q>
    inline auto cross(const glm::qua<T, Q>& q1, const glm::qua<T, Q>& q2) {return glm::cross(q1, q2);}

    template<typename T, glm::qualifier Q>
    inline auto cross(const glm::vec<3, T, Q>& v, const glm::qua<T, Q>& q) {return glm::cross(v, q);}

    template<typename T, glm::qualifier Q>
    inline auto cross(const glm::qua<T, Q>& q, const glm::vec<3, T, Q>& v) {return glm::cross(q, v);}

    // === Explicit sqrt overloads ===    
    template<int L, typename T, glm::qualifier Q>
    inline glm::vec<L, T, Q> sqrt(const glm::vec<L, T, Q>& v) {return glm::sqrt(v);}

    template<typename T, glm::qualifier Q>
    inline glm::qua<T, Q> sqrt(const glm::qua<T, Q>& q) {return glm::sqrt(q);}

    inline int sqrt(int x) {return glm::sqrt(x);}

    inline unsigned int sqrt(unsigned int x) {return glm::sqrt(x);}

    inline float sqrt(float x) {return glm::sqrt(x);}

    // template<typename T, typename U>
    // inline auto dot(T&& a, U&& b) { return glm::dot(std::forward<T>(a), std::forward<U>(b)); }

    // template<typename T>
    // inline auto length(T&& v) { return glm::length(std::forward<T>(v)); }

    // template<typename T>
    // inline auto radians(T&& v) { return glm::radians(std::forward<T>(v)); }

    // template<typename T>
    // inline auto degrees(T&& v) { return glm::degrees(std::forward<T>(v)); }

    template<typename T>
    inline auto value_ptr(T&& v) { return glm::value_ptr(std::forward<T>(v)); }
}