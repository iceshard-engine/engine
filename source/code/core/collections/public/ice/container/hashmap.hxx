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

        template<typename Type, ice::ContainerLogic Logic>
        inline void reserve(ice::HashMap<Type, Logic>& map, ice::u32 new_capacity) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear(ice::HashMap<Type, Logic>& map) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void shrink(ice::HashMap<Type, Logic>& map) noexcept;

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

        template<typename Type, ice::ContainerLogic Logic>
        inline auto values(ice::HashMap<Type, Logic>& map) noexcept -> ice::Span<Type>;


        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto count(HashMapType const& map) noexcept -> ice::u32;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool full(HashMapType const& map) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool empty(HashMapType const& map) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool any(HashMapType const& map) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool has(HashMapType const& map, ice::u64 key) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto get(
            HashMapType const& map,
            ice::u64 key,
            typename HashMapType::ValueType const& fallback_value
        ) noexcept -> typename HashMapType::Value const&;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto get(
            HashMapType const& map,
            ice::u64 key,
            std::nullptr_t
        ) noexcept -> typename HashMapType::ValueType;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto try_get(HashMapType const& map, ice::u64 key) noexcept -> typename HashMapType::ValueType const*;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto begin(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto end(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto values(HashMapType const& map) noexcept -> ice::Span<typename HashMapType::ValueType const>;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto entries(HashMapType const& map) noexcept -> ice::Span<typename HashMapType::Entry const>;


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

namespace ice
{

    using ice::hashmap::begin;
    using ice::hashmap::end;

} // namespace ice

#include "impl/hashmap_impl.inl"
