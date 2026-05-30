/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/math/constants.hxx>

namespace ice::math
{

    struct deg64;
    struct deg32;
    struct rad64;
    struct rad32;

    // TODO: Make a proper "scalar" base-type
    template<typename T>
    concept SimpleNumberType = std::is_arithmetic_v<T>;

    template<typename T>
    concept StrongScalarType = std::is_same_v<T, deg64> || std::is_same_v<T, deg32> || std::is_same_v<T, rad64> || std::is_same_v<T, rad32>;

    struct deg64
    {
        constexpr auto operator*(this deg64 self, SimpleNumberType auto number) noexcept -> deg64 { return { self._value * ice::f64(number) }; }
        constexpr auto operator/(this deg64 self, SimpleNumberType auto number) noexcept -> deg64 { return { self._value / ice::f64(number) }; }
        constexpr auto operator+(this deg64 self, SimpleNumberType auto number) noexcept -> deg64 { return { self._value + ice::f64(number) }; }
        constexpr auto operator-(this deg64 self, SimpleNumberType auto number) noexcept -> deg64 { return { self._value - ice::f64(number) }; }
        constexpr auto operator+(this deg64 self, deg64 other) noexcept -> deg64 { return { self._value + other._value }; }
        constexpr auto operator-(this deg64 self, deg64 other) noexcept -> deg64 { return { self._value - other._value }; }

        constexpr auto operator<=>(this deg64 self, SimpleNumberType auto other) noexcept { return self._value <=> other; }
        constexpr auto operator<=>(this deg64 self, deg64 other) noexcept { return self._value <=> other._value; }

        constexpr auto raw_value(this deg64 self) noexcept -> f64 { return self._value; }
        constexpr auto to_rad64(this deg64 self) noexcept -> rad64;
        constexpr auto to_rad32(this deg64 self) noexcept -> rad32;

        constexpr operator deg32() const noexcept;


        f64 _value;
    };

    struct deg32
    {
        constexpr auto operator*(this deg32 self, SimpleNumberType auto number) noexcept -> deg32 { return { self._value * ice::f32(number) }; }
        constexpr auto operator/(this deg32 self, SimpleNumberType auto number) noexcept -> deg32 { return { self._value / ice::f32(number) }; }
        constexpr auto operator+(this deg32 self, SimpleNumberType auto number) noexcept -> deg32 { return { self._value + ice::f32(number) }; }
        constexpr auto operator-(this deg32 self, SimpleNumberType auto number) noexcept -> deg32 { return { self._value - ice::f32(number) }; }
        constexpr auto operator+(this deg32 self, deg32 other) noexcept -> deg32 { return { self._value + other._value }; }
        constexpr auto operator-(this deg32 self, deg32 other) noexcept -> deg32 { return { self._value - other._value }; }

        constexpr auto operator<=>(this deg32 self, SimpleNumberType auto other) noexcept { return self._value <=> other; }
        constexpr auto operator<=>(this deg32 self, deg32 other) noexcept { return self._value <=> other._value; }

        constexpr auto raw_value(this deg32 self) noexcept -> f32 { return self._value; }
        constexpr auto to_rad64(this deg32 self) noexcept -> rad64;
        constexpr auto to_rad32(this deg32 self) noexcept -> rad32;

        constexpr operator deg64() const noexcept;

        f32 _value;
    };

    struct rad64
    {
        constexpr auto raw_value(this rad64 self) noexcept -> f64 { return self._value; }
        constexpr auto to_rad32(this rad64 self) noexcept -> rad32;
        constexpr auto to_deg64(this rad64 self) noexcept -> deg64;
        constexpr auto to_deg32(this rad64 self) noexcept -> deg32;

        f64 _value;
    };

    struct rad32
    {
        constexpr auto raw_value(this rad32 self) noexcept -> f32 { return self._value; }
        constexpr auto to_rad64(this rad32 self) noexcept -> rad64;
        constexpr auto to_deg64(this rad32 self) noexcept -> deg64;
        constexpr auto to_deg32(this rad32 self) noexcept -> deg32;

        f32 _value;
    };

    // deg64
    inline constexpr auto deg64::to_rad64(this deg64 self) noexcept -> rad64
    {
        return rad64{ self._value * (ice::math::f64_pi / 180.0) };
    }

    inline constexpr auto deg64::to_rad32(this deg64 self) noexcept -> rad32
    {
        return rad32{ ice::f32(self._value) * (ice::math::f32_pi / 180.0f) };
    }

    constexpr deg64::operator deg32() const noexcept
    {
        return { ice::f32(_value) };
    }

    // deg32
    inline constexpr auto deg32::to_rad64(this deg32 self) noexcept -> rad64
    {
        return rad64{ ice::f64(self._value) * (ice::math::f64_pi / 180.0) };
    }

    inline constexpr auto deg32::to_rad32(this deg32 self) noexcept -> rad32
    {
        return rad32{ self._value * (ice::math::f32_pi / 180.0f) };
    }

    constexpr deg32::operator deg64() const noexcept
    {
        return { _value };
    }

    // rad64
    inline constexpr auto rad64::to_rad32(this rad64 self) noexcept -> rad32
    {
        return rad32{ ice::f32(self._value) };
    }

    inline constexpr auto rad64::to_deg64(this rad64 self) noexcept -> deg64
    {
        return deg64{ (self._value * 180.0) / ice::math::f64_pi };
    }

    inline constexpr auto rad64::to_deg32(this rad64 self) noexcept -> deg32
    {
        return deg32{ (ice::f32(self._value) * 180.0f) / ice::math::f32_pi };
    }

    // rad32
    inline constexpr auto rad32::to_rad64(this rad32 self) noexcept -> rad64
    {
        return rad64{ self._value };
    }

    inline constexpr auto rad32::to_deg64(this rad32 self) noexcept -> deg64
    {
        return deg64{ (ice::f64(self._value) * 180.0) / ice::math::f64_pi };
    }

    inline constexpr auto rad32::to_deg32(this rad32 self) noexcept -> deg32
    {
        return deg32{ (self._value * 180.0f) / ice::math::f32_pi };
    }

    inline constexpr auto operator""_deg(long double value) noexcept -> ice::math::deg64
    {
        return { static_cast<ice::f64>(value) };
    }

    // Operators

    template<StrongScalarType ScalarType, SimpleNumberType NumberType>
    inline constexpr auto operator*(ScalarType left, NumberType right) noexcept -> ScalarType
    {
        return { left.raw_value() * right };
    }

    template<StrongScalarType ScalarType, SimpleNumberType NumberType>
    inline constexpr auto operator/(ScalarType left, NumberType right) noexcept -> ScalarType
    {
        return { left.raw_value() / right };
    }

    using deg = deg32;
    using rad = rad32;

} // namespace ice::math
