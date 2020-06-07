#pragma once
#include <core/math/matrix.hxx>

namespace core::math
{

    template<typename T>
    struct mat<1, 1, T>
    {
        using value_type = T;

        union
        {
            T v[1][1];
            T x;
        };
    };

    template<typename T>
    struct mat<2, 1, T>
    {
        using value_type = T;

        union
        {
            T v[1][2];
            struct
            {
                T x, y;
            };
        };
    };

    template<typename T>
    struct mat<3, 1, T>
    {
        using value_type = T;

        union
        {
            T v[1][3];
            struct
            {
                T x, y, z;
            };
        };
    };

    template<typename T>
    struct mat<4, 1, T>
    {
        using value_type = T;

        union
        {
            T v[1][4];
            struct
            {
                T x, y, z, w;
            };
        };
    };

    template<u32 Size, typename T>
    using vec = mat<Size, 1, T>;

    using vec1f = vec<1, f32>;
    using vec1u = vec<1, u32>;
    using vec1i = vec<1, i32>;

    using vec2f = vec<2, f32>;
    using vec2u = vec<2, u32>;
    using vec2i = vec<2, i32>;

    using vec3f = vec<3, f32>;
    using vec3u = vec<3, u32>;
    using vec3i = vec<3, i32>;

    using vec4f = vec<4, f32>;
    using vec4u = vec<4, u32>;
    using vec4i = vec<4, i32>;

    template<typename T>
    constexpr auto vec4(T val = T{ 0 }) noexcept -> vec<4, T>
    {
        return { val, val, val, val };
    }

    template<typename T>
    constexpr auto vec4(T x, T y, T z, T w) noexcept -> vec<4, T>
    {
        return { x, y, z, w };
    }

    template<u32 Size, typename T, typename... U>
    constexpr auto vec4(vec<Size, T> base, U... extended) noexcept -> vec<4, T>
    {
        static_assert(Size + sizeof...(extended) == 4, "Invalid number of components!");

        vec<4, T> result{ };
        T* it = result.v[0];
        for (u32 i = 0; i < Size; ++i)
        {
            *it++ = T{ base.v[0][i] };
        }

        VariadicExpansionContext{
            (*it++ = T{ extended }, 0)...
        };
        return result;
    }

    template<typename T>
    constexpr auto vec3(T val = T{ 0 }) noexcept -> vec<3, T>
    {
        return { val, val, val };
    }

    template<typename T>
    constexpr auto vec3(T x, T y, T z) noexcept -> vec<3, T>
    {
        return { x, y, z };
    }

    template<u32 Size, typename T, typename... U>
    constexpr auto vec3(vec<Size, T> base, U... extended) noexcept -> vec<3, T>
    {
        static_assert(Size + sizeof...(extended) == 4, "Invalid number of components!");

        vec<3, T> result{ };
        T* it = result.v[0];
        for (u32 i = 0; i < Size; ++i)
        {
            *it++ = T{ base.v[0][i] };
        }

        VariadicExpansionContext{
            (*it++ = T{ extended }, 0)...
        };
        return result;
    }

    template<typename T>
    constexpr auto vec3(vec<4, T> base) noexcept -> vec<3, T>
    {
        return { base.v[0][0], base.v[0][1], base.v[0][2] };
    }

    template<typename T>
    constexpr auto vec2(T val = T{ 0 }) noexcept -> vec<2, T>
    {
        return { val, val };
    }

    template<typename T>
    constexpr auto vec2(T x, T y) noexcept -> vec<2, T>
    {
        return { x, y };
    }

    template<typename T>
    constexpr auto vec1(T val = T{ 0 }) noexcept -> vec<1, T>
    {
        return { val };
    }

} // namespace core::math
