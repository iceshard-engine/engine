/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/math/constants.hxx>
#include <cmath>

namespace ice::math
{

    constexpr auto radians(deg degrees) noexcept -> rad;

    constexpr auto degrees(rad radians) noexcept -> deg;

    inline auto sqrt(f32 val) noexcept -> f32;

    inline auto sin(rad radians) noexcept -> f32;

    inline auto cos(rad radians) noexcept -> f32;

    inline auto tan(rad radians) noexcept -> f32;

    inline auto atan2(f32 x, f32 y) noexcept -> f32;


    constexpr auto radians(deg degrees) noexcept -> rad
    {
        return rad{ degrees.value * (ice::math::const_pi / 180.f) };
    }

    constexpr auto degrees(rad radians) noexcept -> deg
    {
        return deg{ (radians.value * 180.f) / ice::math::const_pi };
    }

    inline auto sqrt(f32 val) noexcept -> f32
    {
        return std::sqrt(val);
    }

    inline auto sin(rad radians) noexcept -> f32
    {
        return std::sin(radians.value);
    }

    inline auto cos(rad radians) noexcept -> f32
    {
        return std::cos(radians.value);
    }

    inline auto tan(rad radians) noexcept -> f32
    {
        return std::tan(radians.value);
    }

    inline auto atan2(f32 x, f32 y) noexcept -> f32
    {
        return std::atan2(x, y);
    }

} // namespace ice::math
