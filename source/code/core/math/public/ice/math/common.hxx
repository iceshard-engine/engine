/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/math/constants.hxx>
#include <cmath>

namespace ice::math
{

    constexpr auto radians(deg degrees) noexcept -> rad;

    inline auto sqrt(f32 val) noexcept -> f32;

    inline auto sin(rad radians) noexcept -> f32;

    inline auto cos(rad radians) noexcept -> f32;

    inline auto tan(rad radians) noexcept -> f32;


    constexpr auto radians(deg degrees) noexcept -> rad
    {
        return rad{ degrees.value * (ice::math::const_pi / 180.f) };
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

} // namespace ice::math
