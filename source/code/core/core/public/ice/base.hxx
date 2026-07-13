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

    //! \brief Utility function to access number of elements of a typed C array.
    //!
    //! \tparam T Element type of the C array type being evaluated. <i>(unused by this function)</i>
    //! \tparam Size The size evaluated at compile-time and returned by this function.
    //!
    //! \note This function may have additional overrides for container types.
    //!
    //! \returns Number of elements of a typed C array.
    template<typename T, ice::u32 Size>
    constexpr auto count(T const (&)[Size]) noexcept -> ice::u32
    {
        return Size;
    }

    //! \brief Utility function to check "truthienes" of a value and return its value or a provided default if considered invalid.
    //! \note This function may have multiple overloads and specializations. By default pointer and arithmetic types are supported.
    //!
    //! \param [in] value The value to be checked and returned if considered valid.
    //! \param [in] default_value The fallback value to be returned if \b value is considered invalid.
    //!
    //! \tparam T Type of the \b value parameter.
    //! \tparam U Type of the \b default_value parameter.
    //! 
    //! \pre The \b default_value needs to be convertible to the type of the \b value parameter.
    //! \returns The \b value or \b default_value depending if \b value is considred valid.
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

    //! \brief Adds `const` property to the value protion of the given pointer type
    //! \returns The same pointer value with an added `const` modifier.
    template<typename T>
    constexpr auto to_const(T* value) noexcept -> T const*
    {
        return const_cast<T const*>(value);
    }

    //! \brief Compile-time check returning `true` or `false` depending if a types definition is considered complete.
    template<typename T, typename = void>
    constexpr bool is_type_complete = false;

    template<typename T>
    constexpr bool is_type_complete<T, std::void_t<decltype(sizeof(T))>> = true;

    //! \brief Compile-time utility type to access various information about C++ type members.
    //!
    //! \details This struct allows us to access various information from a C++ member _(method or field)_ and create additional processing
    //!   logic based on these values. This type always provides the \b member_type property which holds information if the accessed data is from
    //!   a method or a field type.
    //!   Method types provide access to:
    //!   * \b class_type - The owning class type. _(does not resolve virtual calls)_
    //!   * \b result_type - The return value type.
    //!   * \b argument_count - Number of all parameters.
    //!   * \b argument_types - Tuple type of all parameter types.
    //!   Field types provide access to:
    //!   * \b class_type - The owning class type. _(does not resolve virtual calls)_
    //!   * \b result_type - The field type.
    //!
    //! \remarks If a non-member type is passed to the \b Member template argument, the \b member_type property will have a value of `0`.
    template<typename Member>
    struct member_info
    {
        //! \brief Category of the passed \b Member type. _(`1` => method, `2` => field, `0` => invalid / not a member type)_
        static constexpr ice::u8 member_type = 0;
    };

    template<typename Class, typename Ret, typename... Args>
    struct member_info<Ret(Class::*)(Args...)>
    {
        static constexpr ice::u8 member_type = 1;

        //! \brief Class type.
        using class_type = Class;
      
        //! \brief Return type.
        using result_type = Ret;

        //! \brief Number of arguments.
        static constexpr ice::u8 argument_count = sizeof...(Args);

        //! \brief Tuple of all arguments types.
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
      
        //! \brief Class type.
        using class_type = Class;

        //! \brief Field type.
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
