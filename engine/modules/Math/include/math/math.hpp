#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

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

    template<typename T>
    inline auto length(T&& v) { return glm::length(std::forward<T>(v)); }

    template<typename T>
    inline auto radians(T&& v) { return glm::radians(std::forward<T>(v)); }

    template<typename T>
    inline auto degrees(T&& v) { return glm::degrees(std::forward<T>(v)); }

    template<class T>
    inline auto perspective(T fovy, T aspect, T zNear, T zFar){return glm::perspective(fovy, aspect, zNear, zFar);}

	template<typename T, glm::qualifier Q>
    inline auto translate(const glm::mat<4, 4, T, Q> & m, const glm::vec<3, T, Q> & v){return glm::translate(m, v);}

	template<typename T, glm::qualifier Q>
    inline auto scale(const glm::mat<4, 4, T, Q> & m, const glm::vec<3, T, Q> & v){return glm::scale(m, v);}

	template<typename T, glm::qualifier Q>
    inline auto toMat4(const glm::qua<T, Q> & x){return glm::toMat4(x);}

    template<class T, glm::qualifier Q>
    inline auto eulerAngles(const glm::qua<T, Q> & x) { return glm::eulerAngles(x); }

	template<typename T, glm::qualifier Q>
    inline auto lerp(glm::qua<T, Q> const& x, glm::qua<T, Q> const& y, T a) {return glm::lerp(x, y, a);}

	template<typename T, glm::qualifier Q>
    inline auto slerp(glm::qua<T, Q> const& x, glm::qua<T, Q> const& y, T a) {return glm::slerp(x, y, a);}

	template<glm::length_t L, typename T, typename U, glm::qualifier Q>
    inline auto mix(glm::vec<L, T, Q> const& x, glm::vec<L, T, Q> const& y, U a) { return glm::mix(x, y, a);}

	template<glm::length_t L, typename T, typename U, glm::qualifier Q>
    inline auto mix(glm::vec<L, T, Q> const& x, glm::vec<L, T, Q> const& y, glm::vec<L, U, Q> const& a) { return glm::mix(x, y, a);}

	template<typename T, glm::qualifier Q>
    inline auto mix(glm::qua<T, Q> const& x, glm::qua<T, Q> const& y, T a) { return glm::mix(x, y, a);};

	template<typename genTypeT, typename genTypeU>
    inline auto mix(genTypeT x, genTypeT y, genTypeU a) { return glm::mix(std::move(x), std::move(y), std::move(a));};

    //template<typename T, glm::qualifier Q>
    //inline auto interpolate(glm::mat<4, 4, T, Q> const& m1, glm::mat<4, 4, T, Q> const& m2, T const delta) {return glm::interpolate(m1, m2, delta);}

    inline constexpr Mat4 interpolate(const Mat4& a, const Mat4& b, float alpha) noexcept
    {
        Mat4 result{};
        for (unsigned int i = 0; i < 4; ++i)
        {
            for (unsigned int j = 0; j < 4; ++j)
            {
                result[i][j] = mix(a[i][j], b[i][j], alpha);
            }
        }
        return result;
    }

    inline constexpr Vec3 extractPosition(const Mat4& mat) noexcept
    {
        return Vec3{ mat[3][0], mat[3][1], mat[3][2] };
    }


	template<typename T, glm::qualifier Q>
    inline auto rotation(glm::vec<3, T, Q> const& orig, glm::vec<3, T, Q> const& dest) {return glm::rotation(orig, dest);}


    template<typename T>
    inline auto value_ptr(T&& v) { return glm::value_ptr(std::forward<T>(v)); }

    inline float cosFromDegrees(float deg) noexcept {
        return glm::cos(glm::radians(deg));
    }

    inline float degreesFromCos(float cosv) noexcept {
        cosv = glm::clamp(cosv, -1.0f, 1.0f);
        return glm::degrees(glm::acos(cosv));
    }


    Vec2Serialized serialize(const Vec2& v);
    Vec3Serialized serialize(const Vec3& v);
    Vec4Serialized serialize(const Vec4& v);
    QuatSerialized serialize(const Quat& v);
    Mat2Serialized serialize(const Mat2& mat2);
    Mat3Serialized serialize(const Mat3& mat3);
    Mat4Serialized serialize(const Mat4& mat4);

    Vec2 deserialize(const Vec2Serialized& v2Serialized);
    Vec3 deserialize(const Vec3Serialized& v3Serialized);
    Vec4 deserialize(const Vec4Serialized& v4Serialized);
    Quat deserialize(const QuatSerialized& qSerialized);
    Mat2 deserialize(const Mat2Serialized& mat2Serialized);
    Mat3 deserialize(const Mat3Serialized& mat3Serialized);
    Mat4 deserialize(const Mat4Serialized& mat4Serialized);
}