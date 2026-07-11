/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/types_extended.hxx>
#include <ice/constants.hxx>
#include <ice/workarounds.hxx>
#include <ice/build/build.hxx>
#include <ice/assert_core.hxx>
#include <ice/utility.hxx>
#include <ice/hash.hxx>

#include <ice/concept/enum_bools.hxx>
#include <ice/concept/enum_flags.hxx>
#include <ice/concept/strong_type_value.hxx>

#include <ice/error.hxx>
#include <ice/error_codes.hxx>

#include <algorithm>
#include <cstring>
#include <utility>
#include <bit>

//! \brief Root namespace for all APIs part of the IceShard engine project.
namespace ice
{

    using std::min;
    using std::max;
    using std::abs;

    using std::swap;
    using std::move;
    using std::forward;
    using std::exchange;
    using std::memcpy;
    using std::memset;
    using std::addressof;
    using std::bit_cast;

    //! \brief Utility function able to return the number of elements of a C array.
    //!
    //! \tparam T Element type of the C array type being evaluated. <i>(unused by this function)</i>
    //! \tparam Size The size evaluated at compile-time and returned by this function.
    //!
    //! \note This function may have additional overrides for container types.
    template<typename T, ice::u32 Size>
    constexpr auto count(T const (&)[Size]) noexcept -> ice::u32
    {
        return Size;
    }

    template<typename T, typename U = T> requires (std::convertible_to<U, T>)
    constexpr auto value_or_default(T value, U default_value) noexcept -> T = delete;

    template<typename T, typename U = T> requires (std::convertible_to<U*, T*>)
    constexpr auto value_or_default(T* value, U* default_value) noexcept -> T*
    {
        return value != nullptr ? value : default_value;
    }

    template<typename T, typename U = T> requires (std::convertible_to<U, T> && std::is_arithmetic_v<T>)
    constexpr auto value_or_default(T value, U&& default_value) noexcept -> T
    {
        return value != nullptr ? value : static_cast<T>(ice::forward<U>(default_value));
    }

    template<typename T>
    using clear_type_t = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;

    template<typename T>
    using clean_type = clear_type_t<T>;

    template<typename T>
    constexpr auto to_const(T* value) noexcept -> T const*
    {
        return const_cast<T const*>(value);
    }

    template<typename T, typename = void>
    constexpr bool is_type_complete = false;

    template<typename T>
    constexpr bool is_type_complete<T, std::void_t<decltype(sizeof(T))>> = true;


    template<typename Member>
    struct member_info
    {
        static constexpr ice::u8 member_type = 0;
    };

    template<typename Class, typename Ret, typename... Args>
    struct member_info<Ret(Class::*)(Args...)>
    {
        static constexpr ice::u8 member_type = 1;
        using class_type = Class;
        using result_type = Ret;

        static constexpr ice::u8 argument_count = sizeof...(Args);
        using argument_types = std::tuple<Args...>;
    };

    template<typename Class, typename Ret, typename... Args>
    struct member_info<Ret(Class::*)(Args...) noexcept>
    {
        static constexpr ice::u8 member_type = 1;
        using class_type = Class;
        using result_type = Ret;

        static constexpr ice::u8 argument_count = sizeof...(Args);
        using argument_types = std::tuple<Args...>;
    };

    template<typename Class, typename Value>
    struct member_info<Value Class::*>
    {
        static constexpr ice::u8 member_type = 2;
        using class_type = Class;
        using result_type = Value;
    };

    template<typename Member>
    using member_class_type_t = typename member_info<Member>::class_type;

    template<typename Member>
    using member_result_type_t = typename member_info<Member>::result_type;

    template<typename Member, ice::u64 Idx>
    using member_arg_type_t = std::tuple_element_t<Idx, typename member_info<Member>::argument_types>;

    template<typename Member>
    constexpr bool is_method_member_v = member_info<Member>::member_type == 1;

    template<typename Member>
    constexpr bool is_field_member_v = member_info<Member>::member_type == 2;

} // namespace ice
