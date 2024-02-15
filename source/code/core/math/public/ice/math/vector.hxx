/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/matrix.hxx>

namespace ice::math
{

    template<typename T>
    struct mat<1, 1, T>
    {
        using value_type = T;
        static constexpr auto count_rows = 1;
        static constexpr auto count_columns = 1;

        constexpr mat() noexcept
            : v{ }
        { }

        explicit constexpr mat(T value) noexcept
            : v{ { value } }
        { }

        union
        {
            T v[count_columns][count_rows];
            T x;
        };
    };

    template<typename T>
    struct mat<2, 1, T>
    {
        using value_type = T;
        static constexpr auto count_rows = 2;
        static constexpr auto count_columns = 1;

        constexpr mat() noexcept
            : v{ }
        { }

        explicit constexpr mat(T value) noexcept
            : v{ { value, value } }
        { }

        constexpr mat(T x, T y) noexcept
            : v{ { x, y } }
        { }

        template<typename U> requires(std::convertible_to<U, T>)
        constexpr explicit mat(mat<2, 1, U> other) noexcept
            : v{ { static_cast<T>(other.x), static_cast<T>(other.y) } }
        { }

        union
        {
            T v[count_columns][count_rows];
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
        static constexpr auto count_rows = 3;
        static constexpr auto count_columns = 1;

        constexpr mat() noexcept
            : v{ }
        { }

        explicit constexpr mat(T value) noexcept
            : v{ { value, value, value } }
        { }

        constexpr mat(T x, T y, T z) noexcept
            : v{ { x, y, z } }
        { }

        template<typename U> requires(std::convertible_to<U, T>)
        constexpr explicit mat(mat<3, 1, U> other) noexcept
            : v{ { static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z) } }
        { }

        union
        {
            T v[count_columns][count_rows];
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
        static constexpr auto count_rows = 4;
        static constexpr auto count_columns = 1;

        constexpr mat() noexcept
            : v{ }
        { }

        explicit constexpr mat(T value) noexcept
            : v{ { value, value, value, value } }
        { }

        constexpr mat(T x, T y, T z, T w) noexcept
            : v{ { x, y, z, w } }
        { }

        template<typename U> requires(std::convertible_to<U, T>)
        constexpr explicit mat(mat<4, 1, U> other) noexcept
            : v{ { static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z), static_cast<T>(other.w) } }
        { }

        union
        {
            T v[count_columns][count_rows];
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

} // namespace ice::math
