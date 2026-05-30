/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/math/angles.hxx>
#include <cmath>

namespace ice::math
{

    constexpr auto radians(deg degrees) noexcept -> rad;

    constexpr auto degrees(rad radians) noexcept -> deg;

    constexpr auto sqrt(f32 val) noexcept -> f32;

    constexpr auto sin(rad radians) noexcept -> f32;

    constexpr auto cos(rad radians) noexcept -> f32;

    constexpr auto tan(rad radians) noexcept -> f32;

    inline auto atan(rad x) noexcept -> f32;

    inline auto atan2(f32 x, f32 y) noexcept -> f32;

    constexpr auto clamp(f32 val, f32 min, f32 max) noexcept -> f32
    {
        return std::max(min, std::min(val, max));
    }

    constexpr auto radians(deg32 degrees) noexcept -> rad
    {
        return degrees.to_rad32();
    }

    constexpr auto radians(deg64 degrees) noexcept -> rad64
    {
        return degrees.to_rad64();
    }

    constexpr auto degrees(rad32 radians) noexcept -> deg
    {
        return radians.to_deg32();
    }

    constexpr auto degrees(rad64 radians) noexcept -> deg64
    {
        return radians.to_deg64();
    }

    constexpr auto sgn(f32 val) noexcept -> f32
    {
        return ice::f32(0.0f < val) - ice::f32(val < 0.0f);
    }

    constexpr auto abs(f32 val) noexcept -> f32
    {
        return val * ice::math::sgn(val);
    }

    inline auto pow(f32 base, f32 exp) noexcept -> f32
    {
        return std::pow(base, exp);
    }

    constexpr auto cbrt(f32 val) noexcept -> f32
    {
        // constexpr cube root using Newton-Raphson
        if consteval
        {
            static constexpr ice::f32 one_over_three = 1.0f / 3.0f;

            if (val == 0.0f)
            {
                return 0.0f;
            }

            ice::f32 guess = abs(val); // initial guess
            for (ice::u32 i = 0; i < 20; ++i)
            {
                guess = (2.0f * guess + val / (guess * guess)) * one_over_three;
            }
            return guess;
        }
        else
        {
            return std::cbrt(val);
        }
    }

    constexpr auto sqrt(f32 val) noexcept -> f32
    {
        // constexpr square root using Newton-Raphson
        if consteval
        {
            if (val < 0.0f)
            {
                return std::numeric_limits<ice::f32>::quiet_NaN();
            }
            else if (val == 0.0f || val == 1.0f)
            {
                return val;
            }

            ice::f32 guess = val * 0.5f; // initial guess
            for (ice::u32 i = 0; i < 20; ++i)
            {
                guess = 0.5f * (guess + val / guess);
            }
            return sgn(val) * guess;
        }
        else
        {
            return std::sqrt(val);
        }
    }

    constexpr auto factorial(u32 n) noexcept -> u32
    {
        if (n <= 1)
        {
            return 1;
        }
        else
        {
            return n * factorial(n - 1);
        }
    }

    constexpr auto sin(rad64 angle) noexcept -> f64
    {
        if consteval
        {
            const f64 muls[]{ 1.0, -1.0 };

            const f64 val = angle.raw_value();
            f64 valp = val;
            f64 fact = 1.0;
            f64 sum = val;
            for (ice::u32 n = 1; n < 23; ++n)
            {
                fact *= (n * 2) * ((n * 2) + 1);
                valp *= val * val;
                sum += muls[(n & 1)] * valp / fact;
            }
            return sum;
        }
        else
        {
            return std::sin(angle.raw_value());
        }
    }

    constexpr auto sin(rad angle) noexcept -> f32
    {
        return f32(ice::math::sin(angle.to_rad64()));
    }

    constexpr auto cos(rad64 angle) noexcept -> f64
    {
        if consteval
        {
            const f64 muls[]{ 1.0, -1.0 };

            const f64 val = angle.raw_value();
            f64 valp = 1.0;
            f64 fact = 1.0;
            f64 sum = 0.0;
            for (ice::u32 n = 1; n < 23; ++n)
            {
                fact *= (n * 2);
                valp *= val * val;
                sum += muls[(n & 1)] * valp / fact;
                fact *= ((n * 2) + 1);
            }
            return sum + 1.0;
        }
        else
        {
            return std::cos(angle.raw_value());
        }
    }

    constexpr auto cos(rad angle) noexcept -> f32
    {
        return f32(ice::math::cos(angle.to_rad64()));
    }

    constexpr auto tan(rad64 radians) noexcept -> f64
    {
        if consteval
        {
            return sin(radians) / cos(radians);
        }
        else
        {
            return std::tan(radians.raw_value());
        }
    }

    constexpr auto tan(rad radians) noexcept -> f32
    {
        if consteval
        {
            return sin(radians) / cos(radians);
        }
        else
        {
            return std::tan(radians.raw_value());
        }
    }

    inline auto atan(rad64 angle) noexcept -> f64
    {
        return std::atan(angle.raw_value());
    }

    inline auto atan(rad x) noexcept -> f32
    {
        return std::atan(x.raw_value());
    }

    inline auto atan2(f64 x, f64 y) noexcept -> f64
    {
        return std::atan2(x, y);
    }

    inline auto atan2(f32 x, f32 y) noexcept -> f32
    {
        return std::atan2(x, y);
    }

} // namespace ice::math
