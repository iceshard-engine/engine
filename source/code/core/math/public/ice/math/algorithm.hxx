/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/constants.hxx>

namespace ice::math
{

    namespace concepts
    {

        template<typename... Args>
        concept is_arithmetic = requires {
            (... && std::is_arithmetic_v<Args>);
        };

    } // namespace concepts

    template<typename T = void, typename U, typename... Args>
        requires concepts::is_arithmetic<U, Args...>
    constexpr auto sum(U first, Args... args) noexcept
    {
        using Target = std::conditional_t<std::is_same_v<T, void>, U, T>;
        return (static_cast<Target>(first) + ... + static_cast<Target>(args));
    }

    template<typename T = void, typename U, typename... Args>
        requires concepts::is_arithmetic<U, Args...>
    constexpr auto sub(U first, Args... args) noexcept
    {
        using Target = std::conditional_t<std::is_same_v<T, void>, U, T>;
        return (static_cast<Target>(first) - ... - static_cast<Target>(args));
    }

    template<typename T = void, typename U, typename... Args>
        requires concepts::is_arithmetic<U, Args...>
    constexpr auto mul(U first, Args... args) noexcept
    {
        using Target = std::conditional_t<std::is_same_v<T, void>, U, T>;
        return (static_cast<Target>(first) * ... * static_cast<Target>(args));
    }

    template<typename T = void, typename U, typename... Args>
        requires concepts::is_arithmetic<U, Args...>
    constexpr auto div(U first, Args... args) noexcept
    {
        using Target = std::conditional_t<std::is_same_v<T, void>, U, T>;
        return (static_cast<Target>(first) / ... / static_cast<Target>(args));
    }

} // namespace ice::math
