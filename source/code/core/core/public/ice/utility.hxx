#pragma once
#include <tuple>

namespace ice
{

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

} // namespace ice
