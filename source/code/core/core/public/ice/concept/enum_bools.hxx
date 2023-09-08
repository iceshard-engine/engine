/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <type_traits>

namespace ice
{

    template<typename T>
    struct BoolLogic
    {
        static constexpr bool IsEnabled = std::is_enum_v<T> && (sizeof(T) == 1);
    };

    template<typename T>
    concept BoolType = BoolLogic<T>::IsEnabled && requires(T) {
        { T::Off } -> std::convertible_to<T>;
        { T::On } -> std::convertible_to<T>;
        static_cast<std::underlying_type_t<T>>(T::Off) == 0;
        static_cast<std::underlying_type_t<T>>(T::On) == 1;
    };

    template<ice::BoolType T>
    constexpr bool operator==(T left, bool right) noexcept
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        return left_value == right;
    }

    template<ice::BoolType T>
    constexpr bool operator!=(T left, bool right) noexcept
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        return left_value != right;
    }

    template<ice::BoolType T>
    constexpr auto operator!(T left) noexcept -> T
    {
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        return static_cast<T>(!left_value);
    }


    namespace detail::internal::static_unit_tests
    {

        enum class FailTest : ice::u8 { Off, On };

        static_assert(FailTest::Off == false);
        static_assert(FailTest::On == true);
        static_assert(FailTest::Off != true);
        static_assert(FailTest::On != false);
        static_assert(!FailTest::Off == true);
        static_assert(!FailTest::On == false);

    } // namespace detail::internal::static_unit_tests

} // namespace ice
