/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/array.hxx>
#include <ice/mem_initializers.hxx>

namespace ice
{

    namespace hashmap
    {

        template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value const& value) noexcept;

        template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
            requires std::move_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value&& value) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
            requires std::copy_constructible<Type>
        inline auto get_or_set(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value_if_missing) noexcept -> Type&;

        template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
            requires std::move_constructible<Type> && std::convertible_to<Value, Type>
        inline auto get_or_set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value&& value_if_missing) noexcept -> Type&;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto try_get(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> Type*;

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;

        template<typename HashMapType>
        inline auto get(
            HashMapType const& map,
            ice::u64 key,
            typename HashMapType::ValueType const& fallback_value
        ) noexcept -> typename HashMapType::ValueType const&;

        template<typename HashMapType>
        inline auto get(
            HashMapType const& map,
            ice::u64 key,
            std::nullptr_t
        ) noexcept -> typename HashMapType::ValueType;

        template<typename HashMapType>
        inline auto try_get(HashMapType const& map, ice::u64 key) noexcept -> typename HashMapType::ValueType const*;


        template<typename Type, ice::ContainerLogic Logic>
        inline auto memory(ice::HashMap<Type, Logic>& map) noexcept -> ice::Memory;

    } // namespace hashmap

    namespace multi_hashmap
    {

        template<typename Type, ice::ContainerLogic Logic>
            requires std::move_constructible<Type> && std::convertible_to<Type, Type>
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type&& value) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator entry) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove_all(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;


        template<typename Type, ice::ContainerLogic Logic>
        inline auto count(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> ice::u32;

        template<typename Type, ice::ContainerLogic Logic>
        inline void get(ice::HashMap<Type, Logic> const& map, ice::u64 key, ice::Array<Type, Logic>& items) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_first(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_next(
            ice::HashMap<Type, Logic> const& map,
            typename ice::HashMap<Type, Logic>::ConstIterator entry
        ) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

    } // namespace multi_hashmap

} // namespace ice

#include "impl/hashmap_impl.inl"
