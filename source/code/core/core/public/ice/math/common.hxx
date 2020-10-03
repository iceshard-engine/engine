#pragma once
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
        return std::sqrtf(val);
    }

    inline auto sin(rad radians) noexcept -> f32
    {
        return std::sinf(radians.value);
    }

    inline auto cos(rad radians) noexcept -> f32
    {
        return std::cosf(radians.value);
    }

    inline auto tan(rad radians) noexcept -> f32
    {
        return std::tanf(radians.value);
    }

} // namespace ice::math
