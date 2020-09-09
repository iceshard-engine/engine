#pragma once
#include <core/math/base.hxx>
#include <core/math/numbers.hxx>

namespace core::math
{

    constexpr auto radians(deg degrees) noexcept -> rad
    {
        return rad{ degrees.value * (core::math::pi / 180.f) };
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

} // namespace core::math
