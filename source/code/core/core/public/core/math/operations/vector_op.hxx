#pragma once
#include <core/math/vector.hxx>

namespace core::math
{

    inline auto normalize(vec3f val) noexcept -> vec3f
    {
        f32 const invsqrt = 1.f / sqrt(val.x * val.x + val.y * val.y + val.z * val.z);
        return { val.x * invsqrt, val.y * invsqrt, val.z * invsqrt };
    }

    constexpr auto cross(vec3f left, vec3f right) noexcept -> vec3f
    {
        return {
            left.y * right.z - right.y * left.z,
            left.z * right.x - right.z * left.x,
            left.x * right.y - right.x * left.y
        };
    }

    template<typename T>
    constexpr auto dot(vec<3, T> left, vec<3, T> right) noexcept -> T
    {
        return {
            left.v[0][0] * right.v[0][0] +
            left.v[0][1] * right.v[0][1] +
            left.v[0][2] * right.v[0][2]
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator+(vec<2, T> left, vec<2, U> right) noexcept -> vec<2, T>
    {
        return {
            left.v[0][0] + right.v[0][0],
            left.v[0][1] + right.v[0][1],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator+(vec<3, T> left, vec<3, U> right) noexcept -> vec<3, T>
    {
        return {
            left.v[0][0] + right.v[0][0],
            left.v[0][1] + right.v[0][1],
            left.v[0][2] + right.v[0][2],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator+(vec<4, T> left, vec<4, U> right) noexcept -> vec<4, T>
    {
        return {
            left.v[0][0] + right.v[0][0],
            left.v[0][1] + right.v[0][1],
            left.v[0][2] + right.v[0][2],
            left.v[0][3] + right.v[0][3],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator-(vec<2, T> left, vec<2, U> right) noexcept -> vec<2, T>
    {
        return {
            left.v[0][0] - right.v[0][0],
            left.v[0][1] - right.v[0][1],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator-(vec<3, T> left, vec<3, U> right) noexcept -> vec<3, T>
    {
        return {
            left.v[0][0] - right.v[0][0],
            left.v[0][1] - right.v[0][1],
            left.v[0][2] - right.v[0][2],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator-(vec<4, T> left, vec<4, U> right) noexcept -> vec<4, T>
    {
        return {
            left.v[0][0] - right.v[0][0],
            left.v[0][1] - right.v[0][1],
            left.v[0][2] - right.v[0][2],
            left.v[0][3] - right.v[0][3],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator*(vec<2, T> left, U scalar) noexcept -> vec<2, T>
    {
        return {
            left.v[0][0] * scalar,
            left.v[0][1] * scalar,
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator*(vec<3, T> left, U scalar) noexcept -> vec<3, T>
    {
        return {
            left.v[0][0] * scalar,
            left.v[0][1] * scalar,
            left.v[0][2] * scalar,
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator*(vec<4, T> left, U scalar) noexcept -> vec<4, T>
    {
        return {
            left.v[0][0] * scalar,
            left.v[0][1] * scalar,
            left.v[0][2] * scalar,
            left.v[0][3],
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator/(vec<2, T> left, U scalar) noexcept -> vec<2, T>
    {
        return {
            left.v[0][0] / scalar,
            left.v[0][1] / scalar,
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator/(vec<3, T> left, U scalar) noexcept -> vec<3, T>
    {
        return {
            left.v[0][0] / scalar,
            left.v[0][1] / scalar,
            left.v[0][2] / scalar,
        };
    }

    template<typename T, typename U = T>
    constexpr auto operator/(vec<4, T> left, U scalar) noexcept -> vec<4, T>
    {
        return {
            left.v[0][0],
            left.v[0][1],
            left.v[0][2],
            left.v[0][3] * scalar,
        };
    }

} // namespace core::math
