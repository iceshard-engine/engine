/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/array.hxx>
#include <numeric>

namespace ice::math
{

    template<u32 Size, typename T, typename U = T>
    constexpr auto max(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto min(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto add(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto add(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto sub(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto sub(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto mul(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto mul(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto mul(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto div(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto div(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto div(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;


    template<u32 Size, typename T, typename U>
    constexpr auto max(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = ice::max(left.v[i], T{ right.v[i] });
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto min(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = ice::min(left.v[i], T{ right.v[i] });
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto add(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = left.v[i] + T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto add(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] + T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto sub(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = left.v[i] - T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto sub(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] - T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto mul(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>
    {
        return mul(left, arr_t<Size, U>{ right });
    }

    template<u32 Size, typename T, typename U>
    constexpr auto mul(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = left.v[i] * T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto mul(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] * T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto div(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>
    {
        return div(left, arr_t<Size, U>{ right });
    }

    template<u32 Size, typename T, typename U>
    constexpr auto div(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = left.v[i] / T{ right.v[i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto div(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] / T{ right.v[i] };
        }
        return result;
    }

} // namespace ice::math
