/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <tuple>

namespace ice
{

    // Const utlities (deducing-this helpers)
    template<typename OwnerT, typename ValueT>
    struct const_correct
    {
        using type = ValueT;
    };

    template<typename OwnerT, typename ValueT>
    struct const_correct<OwnerT const, ValueT>
    {
        using type = std::add_const_t<ValueT>;
    };

    template<typename OwnerT, typename ValueT>
    using const_correct_t = typename ice::const_correct<OwnerT, ValueT>::type;

    // Tuple utilities

    using std::tuple;

    template<typename... Tuples>
    struct tuples_merged;

    template<typename... Tuples>
    using tuples_merged_t = typename ice::tuples_merged<Tuples...>::type;


    template<typename... Types>
    struct tuples_merged<std::tuple<Types...>>
    {
        using type = std::tuple<Types...>;
    };

    template<typename... FirstTypes, typename... SecondTypes, typename... Tail>
    struct tuples_merged<std::tuple<FirstTypes...>, std::tuple<SecondTypes...>, Tail...>
    {
        using type = tuples_merged_t<std::tuple<FirstTypes..., SecondTypes...>, Tail...>;
    };


    template <typename T, typename... Ts>
    struct unique_tuple : std::type_identity<T> {};

    template <typename... Ts, typename U, typename... Us>
    struct unique_tuple<std::tuple<Ts...>, U, Us...>
        : std::conditional_t<(std::is_same_v<U, Ts> || ...)
        , ice::unique_tuple<std::tuple<Ts...>, Us...>
        , ice::unique_tuple<std::tuple<Ts..., U>, Us...>> {};

    template <typename... Ts>
    using unique_tuple_t = typename ice::unique_tuple<std::tuple<>, Ts...>::type;

    template <typename T>
    struct make_unique_tuple_helper;

    template <typename... Ts>
    struct make_unique_tuple_helper<std::tuple<Ts...>>
    {
        using type = typename ice::unique_tuple<std::tuple<>, Ts...>::type;
    };

    template<typename T>
    using make_unique_tuple = typename make_unique_tuple_helper<T>::type;


    // common functions

    namespace concepts
    {
        template<typename Fn, typename T, typename O>
        concept ComparisonFunction = requires(T&& a, O&& b) {
            { std::forward<Fn>(std::declval<Fn>())(std::forward<T>(a), std::forward<O>(b)) } -> std::convertible_to<bool>;
        };
    }

    template<typename Left, typename Right = Left>
    constexpr auto equal(Left&& left, Right&& right) noexcept -> bool
    {
        return std::forward<Left>(left) == std::forward<Left>(right);
    }

    template<typename Left, typename Right = Left>
    constexpr auto less(Left const& left, Right const& right) noexcept -> bool
    {
        return left < right;
    }

    // others

    template <typename Field, typename Class>
    constexpr auto offset_of(Field Class::* member) noexcept -> ice::uptr
    {
        return reinterpret_cast<ice::uptr>(
            &(((Class*)0)->*member)
        );
    }

} // namespace ice
