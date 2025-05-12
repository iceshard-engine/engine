/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <type_traits>

namespace ice
{

    template<typename T>
    struct FlagLogic
    {
        static constexpr bool IsEnabled = std::is_enum_v<T>;
    };

    template<typename T>
    concept FlagType = FlagLogic<T>::IsEnabled && requires(T) {
        { T::None } -> std::convertible_to<T>;
    };

    template<typename T>
    concept FlagAllValue = FlagType<T> && requires(T) {
        { T::All } -> std::convertible_to<T>;
    };

    template<ice::FlagType T>
    constexpr auto operator|(T left, T right) noexcept -> T
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        auto const right_value = static_cast<std::underlying_type_t<T>>(right);
        return static_cast<T>(left_value | right_value);
    }

    template<ice::FlagType T>
    constexpr auto operator|=(T& left, T right) noexcept -> T&
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        auto const right_value = static_cast<std::underlying_type_t<T>>(right);
        left = static_cast<T>(left_value | right_value);
        return left;
    }

    template<ice::FlagType T>
    constexpr auto operator&(T left, T right) noexcept -> T
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        auto const right_value = static_cast<std::underlying_type_t<T>>(right);
        return static_cast<T>(left_value & right_value);
    }

    template<ice::FlagType T>
    constexpr auto operator&=(T& left, T right) noexcept -> T&
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        auto const right_value = static_cast<std::underlying_type_t<T>>(right);
        left = static_cast<T>(left_value & right_value);
        return left;
    }

    template<ice::FlagType T>
    constexpr auto operator~(T left) noexcept -> T
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        if constexpr (FlagAllValue<T>)
        {
            auto const all_value = static_cast<std::underlying_type_t<T>>(T::All);
            return static_cast<T>(left_value ^ all_value);
        }
        else
        {
            return static_cast<T>(~left_value);
        }
    }

    template<ice::FlagType T>
    constexpr bool has_all(T value, T expected_flags) noexcept
    {
        return (value & expected_flags) == expected_flags;
    }

    template<ice::FlagType T>
    constexpr bool has_any(T value, T expected_flags) noexcept
    {
        return (value & expected_flags) != T::None;
    }

    template<ice::FlagType T>
    constexpr bool has_none(T value, T expected_flags) noexcept
    {
        return (value & expected_flags) == T::None;
    }

} // namespace ice
