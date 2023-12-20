/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/types_extended.hxx>
#include <ice/constants.hxx>
#include <ice/workarounds.hxx>
#include <ice/build/build.hxx>
#include <ice/assert_core.hxx>
#include <ice/config.hxx>
#include <ice/hash.hxx>

#include <ice/concept/enum_bools.hxx>
#include <ice/concept/enum_flags.hxx>
#include <ice/concept/strong_type_value.hxx>

#include <algorithm>
#include <cstring>
#include <utility>
#include <bit>

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

    template<typename T, ice::u32 Size>
    constexpr auto count(T const (&)[Size]) noexcept -> ice::u32
    {
        return Size;
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

    template<typename Member>
    constexpr bool is_method_member_v = member_info<Member>::member_type == 1;

    template<typename Member>
    constexpr bool is_field_member_v = member_info<Member>::member_type == 2;

} // namespace ice
