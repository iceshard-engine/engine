#pragma once
#include <core/math/base.hxx>
#include <core/math/numbers.hxx>

namespace core::math
{

    constexpr auto radians(f32 degrees) noexcept -> f32
    {
        return degrees * (core::math::pi / 180.f);
    }

    inline auto sqrt(f32 val) noexcept -> f32
    {
        return std::sqrtf(val);
    }

    inline auto sin(f32 radians) noexcept -> f32
    {
        return std::sinf(radians);
    }

    inline auto cos(f32 radians) noexcept -> f32
    {
        return std::cosf(radians);
    }

} // namespace core::math
