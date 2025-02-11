/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/vector.hxx>
#include <numeric>

namespace ice::math_detail
{

    template<u32 Size, typename T, size_t... Values>
    constexpr auto length2_sequenced(ice::math::vec<Size, T> vector, std::index_sequence<Values...>) noexcept -> T
    {
        return ((vector.v[0][Values] * vector.v[0][Values]) + ... + T{});
    }

    template<u32 Size, typename T, size_t... Values>
    constexpr auto length2_sequenced(T(&array)[Size], std::index_sequence<Values...>) noexcept -> T
    {
        return ((array[Values] * array[Values]) + ... + T{});
    }

} // namespace detail

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

    template<u32 Size, typename T>
    constexpr auto length(vec<Size, T> left) noexcept -> T;

    template<u32 Size, typename T>
    constexpr auto length2(vec<Size, T> left) noexcept -> T;

    constexpr auto cross(vec<3, f32> left, vec<3, f32> right) noexcept -> vec<3, f32>;

    constexpr auto dot(vec<3, f32> left, vec<3, f32> right) noexcept -> f32;

    inline auto atan2(vec<2, f32> point) noexcept -> rad;

    template<u32 Size, typename T> requires(Size > 1)
    inline auto normalize(vec<Size, T> value) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto apply(vec<Size, T> left, U(*fn)(T) noexcept) noexcept -> vec<Size, U>;


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

    template<u32 Size, typename T>
    constexpr auto length2(vec<Size, T> left) noexcept -> T
    {
        return ice::math_detail::length2_sequenced(left, std::make_index_sequence<Size>{});
    }

    template<u32 Size, typename T>
    constexpr auto length(vec<Size, T> left) noexcept -> T
    {
        return ice::math::sqrt(ice::math::length2(left));
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

    inline auto atan2(vec<2, f32> point) noexcept -> rad
    {
        return { ice::math::atan2(point.v[0][0], point.v[0][1]) };
    }

    template<u32 Size, typename T> requires(Size > 1)
    inline auto normalize(vec<Size, T> value) noexcept -> vec<Size, T>
    {
        if constexpr (Size == 4)
        {
            ICE_ASSERT_CORE(value.v[0][3] != 0.f);
            ice::math::div(value, value.v[0][3]);
        }

        f32 const square_sum = ice::math::length2(value);
        if (square_sum == 0)
        {
            return value;
        }

        f32 const sqrt_inverted = 1.f / sqrt(square_sum);
        return mul(value, sqrt_inverted);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto apply(vec<Size, T> left, U(*fn)(T) noexcept) noexcept -> vec<Size, U>
    {
        vec<Size, U> result;
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = fn(left.v[0][i]);
        }
        return result;
    }

} // namespace ice::math
