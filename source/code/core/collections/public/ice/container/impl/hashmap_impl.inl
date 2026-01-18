/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    namespace hashmap::detail
    {

        struct FindResult;

        constexpr auto calc_storage_capacity(ice::u32 max_count) noexcept -> ice::u32;

        template<typename Type, ice::ContainerLogic Logic>
        inline bool can_store_count(ice::HashMap<Type, Logic> const& map, ice::u32 expected_count) noexcept;

        template<typename HashMapType>
        inline auto find(HashMapType const& map, ice::u64 key) noexcept -> FindResult;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto make(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> ice::u32;

        template<typename Type, ice::ContainerLogic Logic>
        inline void erase(ice::HashMap<Type, Logic>& map, FindResult const fr) noexcept;

        template<typename HashMapType>
        inline auto find_or_fail(HashMapType const& map, ice::u64 key) noexcept -> ice::u32;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_or_make(ice::HashMap<Type, Logic>& map, ice::u64 key, bool& found) noexcept -> ice::u32;

        template<typename Type, ice::ContainerLogic Logic>
        inline void find_and_erase(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator it) noexcept -> FindResult;

        template<typename Type, ice::ContainerLogic Logic>
        inline void grow(ice::HashMap<Type, Logic>& map) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear_and_dealloc(ice::HashMap<Type, Logic>& map) noexcept;

    } // namespace hashmap::detail


    namespace hashmap
    {

        template<typename Type, ice::ContainerLogic Logic, typename Value>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value const& value) noexcept
        {
            if (map.is_full())
            {
                map.grow();
            }

            bool found = false;
            ice::u32 const index = ice::detail::hashmap::find_or_make(map, key, found);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                // If the index was found we need to destroy the previous value.
                if (found)
                {
                    ice::mem_destruct_at(map._data + index);
                }

                ice::mem_copy_construct_at(
                    Memory{
                        .location = map._data + index,
                        .size = ice::size_of<Type>,
                        .alignment = ice::align_of<Type>
                    },
                    value
                );
            }
            else
            {
                map._data[index] = value;
            }
        }

        template<typename Type, ice::ContainerLogic Logic, typename Value>
            requires std::move_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value&& value) noexcept
        {
            if (map.is_full())
            {
                map.grow();
            }

            bool found = false;
            ice::u32 const index = ice::detail::hashmap::find_or_make(map, key, found);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                // If the index was found we need to destroy the previous value.
                if (found)
                {
                    ice::mem_destruct_at(map._data + index);
                }

                ice::mem_move_construct_at(
                    Memory{
                        .location = map._data + index,
                        .size = ice::size_of<Type>,
                        .alignment = ice::align_of<Type>
                    },
                    ice::forward<Value>(value)
                );
            }
            else
            {
                map._data[index] = value;
            }
        }

        template<typename Type, ice::ContainerLogic Logic, typename Value>
            requires std::move_constructible<Type> && std::convertible_to<Value, Type>
        inline auto get_or_set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value&& value) noexcept -> Type&
        {
            if (map.missing(key))
            {
                ice::hashmap::set(map, key, ice::forward<Value>(value));
            }

            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            ICE_ASSERT_CORE(index != ice::detail::hashmap::Constant_EndOfList);
            return *(map._data + index);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto try_get(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> Type*
        {
            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            return index == ice::detail::hashmap::Constant_EndOfList
                ? nullptr
                : map._data + index;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept
        {
            ice::detail::hashmap::find_and_erase(map, key);
        }

        template<typename HashMapType>
        inline auto get(HashMapType const& map, ice::u64 key, typename HashMapType::ValueType const& fallback_value) noexcept -> typename HashMapType::ValueType const&
        {
            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            return index == ice::detail::hashmap::Constant_EndOfList
                ? fallback_value
                : map._data[index];
        }

        template<typename HashMapType>
        inline auto get(HashMapType const& map, ice::u64 key, std::nullptr_t) noexcept -> typename HashMapType::ValueType
        {
            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            return index == ice::detail::hashmap::Constant_EndOfList
                ? nullptr
                : map._data[index];
        }

        template<typename HashMapType>
        inline auto try_get(HashMapType const& map, ice::u64 key) noexcept -> typename HashMapType::ValueType const*
        {
            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            return index == ice::detail::hashmap::Constant_EndOfList
                ? nullptr
                : map._data + index;
        }



        template<typename Type, ice::ContainerLogic Logic>
        inline auto memory(ice::HashMap<Type, Logic>& map) noexcept -> ice::Memory
        {
            ice::u32 const capacity_values = ice::u32(map._capacity * ice::detail::hashmap::Constant_HashMapMaxFill);

            // TODO: Easier way to calculate the allocated size.
            ice::meminfo alloc_info = ice::meminfo_of<ice::u32> * map._capacity;
            alloc_info += ice::meminfo_of<typename ice::HashMap<Type, Logic>::Entry> * capacity_values;
            alloc_info += ice::meminfo_of<Type> * capacity_values;

            return Memory{
                .location = map._hashes,
                .size = alloc_info.size,
                .alignment = alloc_info.alignment
            };
        }

    } // namespace hashmap

    namespace multi_hashmap
    {

        template<typename Type, ice::ContainerLogic Logic>
            requires std::move_constructible<Type> && std::convertible_to<Type, Type>
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type&& value) noexcept
        {
            if (map.is_full())
            {
                map.grow();
            }

            ice::u32 const index = ice::detail::hashmap::make(map, key);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                // If the index is below the current map._count we need to destroy the previous value.
                if ((index + 1) < map._count)
                {
                    ice::mem_destruct_at(map._data + index);
                }

                ice::mem_move_construct_at(
                    Memory{
                        .location = map._data + index,
                        .size = ice::size_of<Type>,
                        .alignment = ice::align_of<Type>
                    },
                    ice::forward<Type>(value)
                );
            }
            else
            {
                map._data[index] = value;
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value) noexcept
        {
            if (map.is_full())
            {
                map.grow();
            }

            ice::u32 const index = ice::detail::hashmap::make(map, key);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                // If the index is below the current map._count we need to destroy the previous value.
                if ((index + 1) < map._count)
                {
                    ice::mem_destruct_at(map._data + index);
                }

                ice::mem_copy_construct_at(
                    Memory{
                        .location = map._data + index,
                        .size = ice::size_of<Type>,
                        .alignment = ice::align_of<Type>
                    },
                    value
                );
            }
            else
            {
                map._data[index] = value;
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator it) noexcept
        {
            ice::detail::hashmap::FindResult const fr = ice::detail::hashmap::find(map, it);
            if (fr.entry_i != ice::detail::hashmap::Constant_EndOfList)
            {
                ice::detail::hashmap::erase(map, fr);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove_all(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept
        {
            while (map.has(key))
            {
                ice::hashmap::remove(map, key);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto count(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> ice::u32
        {
            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;

            ice::u32 result = 0;
            ConstIterator it = ice::multi_hashmap::find_first(map, key);
            while (it._entry != nullptr)
            {
                result += 1;
                it = ice::multi_hashmap::find_next(map, it);
            }
            return result;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void get(ice::HashMap<Type, Logic> const& map, ice::u64 key, ice::Array<Type, Logic>& items) noexcept
        {
            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;
            ConstIterator it = ice::multi_hashmap::find_first(map, key);

            while (it._entry != nullptr)
            {
                items.push_back(*it._value);
                it = ice::multi_hashmap::find_next(map, it);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_first(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator
        {
            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;

            ice::u32 const index = ice::detail::hashmap::find_or_fail(map, key);
            if (index == ice::detail::hashmap::Constant_EndOfList)
            {
                return ConstIterator{ nullptr, nullptr };
            }
            else
            {
                return ConstIterator{ map._entries + index, map._data + index };
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_next(
            ice::HashMap<Type, Logic> const& map,
            typename ice::HashMap<Type, Logic>::ConstIterator it
        ) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator
        {
            ICE_ASSERT_CORE(it._entry != nullptr && it._value != nullptr);

            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;

            ice::u32 index = it._entry->next;
            while (index != ice::detail::hashmap::Constant_EndOfList)
            {
                if (map._entries[index].key == it._entry->key)
                {
                    return ConstIterator{ map._entries + index, map._data + index };
                }
                index = map._entries[index].next;
            }

            return ConstIterator{ nullptr, nullptr };
        }

    } // namespace multi_hash

} // namespace ice
