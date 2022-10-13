#pragma once
#include <ice/container_types.hxx>
#include <ice/container/array.hxx>
#include <ice/mem_initializers.hxx>

namespace ice
{

    namespace hashmap
    {

        template<typename Type, ice::CollectionLogic Logic>
        inline void reserve(ice::HashMap<Type, Logic>& map, ice::ucount new_capacity) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline void clear(ice::HashMap<Type, Logic>& map) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline void shrink(ice::HashMap<Type, Logic>& map) noexcept;

        template<typename Type, ice::CollectionLogic Logic, typename Value = Type>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value const& value) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
            requires std::move_constructible<Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Type&& value) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
            requires std::copy_constructible<Type>
        inline auto get_or_set(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value_if_missing) noexcept -> Type&;

        template<typename Type, ice::CollectionLogic Logic>
            requires std::move_constructible<Type>
        inline auto get_or_set(ice::HashMap<Type, Logic>& map, ice::u64 key, Type&& value_if_missing) noexcept -> Type&;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto try_get(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> Type*;

        template<typename Type, ice::CollectionLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto values(ice::HashMap<Type, Logic>& map) noexcept -> ice::Span<Type>;


        template<typename Type, ice::CollectionLogic Logic>
        inline bool full(ice::HashMap<Type, Logic> const& map) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline bool empty(ice::HashMap<Type, Logic> const& map) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline bool has(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto get(ice::HashMap<Type, Logic> const& map, ice::u64 key, Type const& fallback_value) noexcept -> Type const&;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto get(ice::HashMap<Type*, Logic> const& map, ice::u64 key, std::nullptr_t) noexcept -> Type*;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto try_get(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> Type const*;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto begin(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto end(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto values(ice::HashMap<Type, Logic> const& map) noexcept -> ice::Span<Type const>;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto entries(ice::HashMap<Type, Logic> const& map) noexcept -> ice::Span<typename ice::HashMap<Type, Logic>::Entry const>;


        template<typename Type, ice::CollectionLogic Logic>
        inline auto memory(ice::HashMap<Type, Logic>& map) noexcept -> ice::Memory;

    } // namespace hashmap

    namespace multi_hashmap
    {

        template<typename Type, ice::CollectionLogic Logic>
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator entry) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline void remove_all(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;


        template<typename Type, ice::CollectionLogic Logic>
        inline auto count(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> ice::ucount;

        template<typename Type, ice::CollectionLogic Logic>
        inline void get(ice::HashMap<Type, Logic> const& map, ice::u64 key, ice::Array<Type, Logic>& items) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto find_first(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator;

        template<typename Type, ice::CollectionLogic Logic>
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
