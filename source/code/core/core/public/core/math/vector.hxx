#pragma once
#include <core/math/base.hxx>

namespace core::math
{

    template<uint32_t Size, typename T>
    struct vec
    {
        T v[Size];
    };

    template<typename T>
    struct vec<1, T>
    {
        T x;

        vec() noexcept = default;
        vec(T x) noexcept
            : x{ x }
        { }
    };

    using vec1 = vec<1, f32>;
    using uvec1 = vec<1, u32>;
    using ivec1 = vec<1, i32>;

    template<typename T>
    struct vec<2, T>
    {
        T x, y;

        vec() noexcept = default;
        vec(T x, T y) noexcept
            : x{ x }
            , y{ y }
        { }
    };

    using vec2 = vec<2, f32>;
    using uvec2 = vec<2, u32>;
    using ivec2 = vec<2, i32>;

    template<typename T>
    struct vec<3, T>
    {
        T x, y, z;

        vec() noexcept = default;
        vec(T x, T y, T z) noexcept
            : x{ x }
            , y{ y }
            , z{ z }
        { }
    };

    using vec3 = vec<3, f32>;
    using uvec3 = vec<3, u32>;
    using ivec3 = vec<3, i32>;

    template<typename T>
    struct vec<4, T>
    {
        T x, y, z, w;

        vec() noexcept = default;
        vec(T x, T y, T z, T w) noexcept
            : x{ x }
            , y{ y }
            , z{ z }
            , w{ w }
        { }
    };

    using vec4 = vec<4, f32>;
    using uvec4 = vec<4, u32>;
    using ivec4 = vec<4, i32>;

} // namespace core::math
