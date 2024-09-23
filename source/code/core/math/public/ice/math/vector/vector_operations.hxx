/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/vector.hxx>
#include <numeric>

namespace ice::math
{

    template<u32 Size, typename T, typename U = T>
    constexpr auto add(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto sub(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto mul(vec<Size, T> left, U right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto div(vec<Size, T> left, U right) noexcept -> vec<Size, T>;

    constexpr auto cross(vec<3, f32> left, vec<3, f32> right) noexcept -> vec<3, f32>;

    constexpr auto dot(vec<3, f32> left, vec<3, f32> right) noexcept -> f32;

    template<u32 Size, typename T> requires(Size > 1)
    inline auto normalize(vec<Size, T> value) noexcept -> vec<Size, T>;


    template<u32 Size, typename T, typename U>
    constexpr auto add(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] + T{ right.v[0][i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto sub(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] - T{ right.v[0][i] };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto mul(vec<Size, T> left, U right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] * T{ right };
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto div(vec<Size, T> left, U right) noexcept -> vec<Size, T>
    {
        vec<Size, T> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = left.v[0][i] / T{ right };
        }
        return result;
    }

    constexpr auto cross(vec<3, f32> left, vec<3, f32> right) noexcept -> vec<3, f32>
    {
        return {
            left.v[0][1] * right.v[0][2] - right.v[0][1] * left.v[0][2],
            left.v[0][2] * right.v[0][0] - right.v[0][2] * left.v[0][0],
            left.v[0][0] * right.v[0][1] - right.v[0][0] * left.v[0][1],
        };
    }

    constexpr auto dot(vec<3, f32> left, vec<3, f32> right) noexcept -> f32
    {
        return left.v[0][0] * right.v[0][0] +
            left.v[0][1] * right.v[0][1] +
            left.v[0][2] * right.v[0][2];
    }

    template<u32 Size, typename T> requires(Size > 1)
    inline auto normalize(vec<Size, T> value) noexcept -> vec<Size, T>
    {
        f32 square_sum = 0;
        for (ucount row = 0; row < Size; ++row)
        {
            square_sum += value.v[0][row] * value.v[0][row];
        }

        f32 const sqrt_inverted = 1.f / sqrt(square_sum);
        return mul(value, sqrt_inverted);
    }

} // namespace ice::math
