/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/vector.hxx>

namespace ice::math
{

    template<u32 Size, typename T>
    struct arr_t;

    template<u32 Size, typename T, typename U = T>
    constexpr auto arr(arr_t<Size, U> arr_val) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T>
    constexpr auto arr(vec<Size, T> vec_val) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto arr(vec<Size, U> vec_val) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T>
    constexpr auto as_vec(vec<Size, T> vec_val) noexcept -> vec<Size, T>;


    template<u32 Size, typename T>
    struct arr_t { };

    template<typename T>
    struct arr_t<1, T>
    {
        using value_type = T;
        static constexpr auto count_elements = 1;

        constexpr arr_t() noexcept
            : v{ }
        { }

        explicit constexpr arr_t(T value) noexcept
            : v{ { value } }
        { }

        union
        {
            T v[count_elements];
            struct
            {
                T x;
            };
        };
    };

    template<typename T>
    struct arr_t<2, T>
    {
        using value_type = T;
        static constexpr auto count_elements = 2;

        constexpr arr_t() noexcept
            : v{ }
        { }

        explicit constexpr arr_t(T value) noexcept
            : v{ value, value }
        { }

        constexpr arr_t(T x, T y) noexcept
            : v{ x, y }
        { }

        union
        {
            T v[count_elements];
            struct
            {
                T x, y;
            };
        };
    };

    template<typename T>
    struct arr_t<3, T>
    {
        using value_type = T;
        static constexpr auto count_elements = 3;

        constexpr arr_t() noexcept
            : v{ }
        { }

        explicit constexpr arr_t(T value) noexcept
            : v{ value, value, value }
        { }

        constexpr arr_t(T x, T y, T z) noexcept
            : v{ x, y, z }
        { }

        union
        {
            T v[count_elements];
            struct
            {
                T x, y, z;
            };
        };
    };

    template<typename T>
    struct arr_t<4, T>
    {
        using value_type = T;
        static constexpr auto count_elements = 4;

        constexpr arr_t() noexcept
            : v{ }
        { }

        explicit constexpr arr_t(T value) noexcept
            : v{ value, value, value, value }
        { }

        constexpr arr_t(T x, T y, T z, T w) noexcept
            : v{ x, y, z, w }
        { }

        union
        {
            T v[count_elements];
            struct
            {
                T x, y, z, w;
            };
        };
    };


    template<u32 Size, typename T, typename U>
    constexpr auto arr(arr_t<Size, U> arr_val) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result{ };
        for (u32 idx = 0; idx < Size; ++idx)
        {
            result.v[idx] = T{ arr_val.v[idx] };
        }
        return result;
    }

    template<u32 Size, typename T>
    constexpr auto arr(vec<Size, T> vec_val) noexcept -> arr_t<Size, T>
    {
        return arr<Size, T, T>(vec_val);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto arr(vec<Size, U> vec_val) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result{ };
        for (u32 idx = 0; idx < Size; ++idx)
        {
            result.v[idx] = T{ vec_val.v[0][idx] };
        }
        return result;
    }

    template<u32 Size, typename T>
    constexpr auto as_vec(arr_t<Size, T> arr_val) noexcept -> vec<Size, T>
    {
        vec<Size, T> result{ };
        for (u32 idx = 0; idx < Size; ++idx)
        {
            result.v[0][idx] = arr_val.v[idx];
        }
        return result;
    }


    using arr1f = arr_t<1, f32>;
    using arr1u = arr_t<1, u32>;
    using arr1i = arr_t<1, i32>;

    using arr2f = arr_t<2, f32>;
    using arr2u = arr_t<2, u32>;
    using arr2i = arr_t<2, i32>;

    using arr3f = arr_t<3, f32>;
    using arr3u = arr_t<3, u32>;
    using arr3i = arr_t<3, i32>;

    using arr4f = arr_t<4, f32>;
    using arr4u = arr_t<4, u32>;
    using arr4i = arr_t<4, i32>;

} // namespace ice::math
