#pragma
#include <ice/array.hxx>
#include <ice/container_logic.hxx>
#include <ice/container/container_concepts.hxx>

namespace ice
{

    namespace container
    {
    }

    namespace concepts::hashmap
    {

    } // namespace concepts

    namespace detail::hashmap
    {

        template<ice::concepts::AssociativeContainerType T>
        using HashMapEntryType = ice::const_correct_t<T, typename std::remove_reference_t<T>::EntryType>;

        //! \brief A concept used to enable access to read-only operations for all compatible types.
        template<typename T>
        concept HashMapContainer = ice::concepts::AssociativeContainerType<T> && requires(T t) {
            typename std::remove_reference_t<T>::EntryType;
            { std::remove_reference_t<T>::OperationLogic } -> std::convertible_to<ice::ContainerLogic>;
            { t._capacity } -> std::convertible_to<ice::u32>;
            { t._count } -> std::convertible_to<ice::u32>;
            { t._hashes } -> std::convertible_to<ice::u32 const*>;
            { t._entries } -> std::convertible_to<ice::detail::hashmap::HashMapEntryType<T>*>;
            { t._data } -> std::convertible_to<ice::container::ValuePtr<T>>;
        };

        static constexpr ice::f32 Constant_HashMapMaxFill = 0.7f;
        static constexpr ice::u32 Constant_EndOfList = 0xffffffffu;

        struct FindResult
        {
            ice::u32 hash_i;
            ice::u32 entry_prev;
            ice::u32 entry_i;
        };

        constexpr auto calc_value_capacity(ice::ncount raw_capacity) noexcept -> ice::ncount
        {
            return static_cast<ice::ncount::base_type>(raw_capacity.native() * Constant_HashMapMaxFill);
        }

        constexpr auto calc_required_capacity(ice::ncount max_count) noexcept -> ice::ncount
        {
            return static_cast<ice::ncount::base_type>(
                max_count.native() / Constant_HashMapMaxFill + 0.99f /* magic */
            );
        }

        constexpr auto capacity_with_overhead(ice::ncount max_count) noexcept -> ice::ncount
        {
            return calc_required_capacity(max_count);
        }

        constexpr bool can_store_expected_size(ice::ncount raw_capacity, ice::ncount expected_size) noexcept
        {
            return calc_value_capacity(raw_capacity) >= expected_size;
        }

        template<typename EntryType, typename ValueType>
        constexpr auto calc_meminfo(ice::ncount capacity) noexcept -> ice::meminfo
        {
            ice::ncount const new_internal_capacity = ice::detail::hashmap::capacity_with_overhead(capacity);

            ice::meminfo alloc_info = ice::meminfo_of<ice::u32> * new_internal_capacity;
            alloc_info += ice::meminfo_of<EntryType> * capacity;
            alloc_info += ice::meminfo_of<ValueType> * capacity;
            return alloc_info;
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto entries(ContainerT const& map) noexcept -> ice::Span<typename ContainerT::EntryType const>
        {
            return ice::Span{ map._entries, map._count };
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto find(ContainerT const& map, ice::u64 key) noexcept -> FindResult
        {
            FindResult fr{
                .hash_i = Constant_EndOfList,
                .entry_prev = Constant_EndOfList,
                .entry_i = Constant_EndOfList,
            };

            if (map._count == 0)
            {
                return fr;
            }

            fr.hash_i = key % map._capacity;
            fr.entry_i = map._hashes[fr.hash_i];

            while (fr.entry_i != Constant_EndOfList)
            {
                if (map._entries[fr.entry_i].key == key)
                {
                    return fr;
                }

                fr.entry_prev = fr.entry_i;
                fr.entry_i = map._entries[fr.entry_i].next;
            }
            return fr;
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto make(ContainerT& map, ice::u64 key) noexcept -> ice::u32
        {
            FindResult fr = ice::detail::hashmap::find(map, key);
            if (fr.hash_i == Constant_EndOfList)
            {
                fr.hash_i = key % map._capacity;
            }

            // The count is now the new index.
            ice::u32 const index = map._count;

            // Set the key we are use to make the new entry.
            map._entries[index].key = key;

            // ... and the next to the previous index stored in the hashes
            map._entries[index].next = map._hashes[fr.hash_i];

            // ... then this is the first entry on this hash table.
            map._hashes[fr.hash_i] = index;

            // ... increase the entry count
            map._count += 1;

            return index;
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline void erase(ContainerT& map, FindResult const fr) noexcept
        {
            using Entry = typename ContainerT::EntryType;
            using Type = typename ContainerT::ValueType;

            // We only update the hash index if we remove the first entry.
            if (fr.entry_prev == Constant_EndOfList)
            {
                map._hashes[fr.hash_i] = map._entries[fr.entry_i].next;
            }
            else
            {
                map._entries[fr.entry_prev].next = map._entries[fr.entry_i].next;
            }

            // Destroy the object...
            if constexpr (ContainerT::OperationLogic == ContainerLogic::Complex)
            {
                ice::mem_destruct_at(map._data + fr.entry_i);
            }

            map._count -= 1;
            if (fr.entry_i == map._count)
            {
                return;
            }

            if constexpr (ContainerT::OperationLogic == ContainerLogic::Complex)
            {
                // Move construct the last object to the now empty location...
                ice::mem_move_construct_at(
                    Memory{
                        .location = map._data + fr.entry_i,
                        .size = ice::size_of<Type>,
                        .alignment = ice::align_of<Type>
                    },
                    ice::move(map._data[map._count])
                );

                // ... then destroy the object at the last location.
                ice::mem_destruct_at(map._data + map._count);
            }
            else
            {
                map._data[fr.entry_i] = map._data[map._count];
            }

            // Copy entry data...
            map._entries[fr.entry_i] = map._entries[map._count];

            // The first and last entry are the same...
            FindResult const last_key_first_entry = ice::detail::hashmap::find(map, map._entries[map._count].key);
            if (last_key_first_entry.entry_prev == Constant_EndOfList)
            {
                // ... update the hashes
                map._hashes[last_key_first_entry.hash_i] = fr.entry_i;
            }
            else
            {
                Entry* prev_entry = map._entries + last_key_first_entry.entry_prev;

                // Until 'next' holds the last value index...
                while (prev_entry->next != map._count)
                {
                    // ... we move forward
                    prev_entry = map._entries + prev_entry->next;
                }

                // ... we can fix the 'next' value
                prev_entry->next = fr.entry_i;
            }
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto find_or_fail(ContainerT const& map, ice::container::KeyType<ContainerT> key) noexcept -> ice::u32
        {
            return ice::detail::hashmap::find(map, key).entry_i;
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto find_or_make(ContainerT& map, ice::u64 key, bool& found) noexcept -> ice::u32
        {
            FindResult fr = ice::detail::hashmap::find(map, key);

            // If entry index is not valid we still might have a previous element on the same hash index.
            if (fr.entry_i != Constant_EndOfList)
            {
                found = true;
                return fr.entry_i;
            }

            if (fr.hash_i == Constant_EndOfList)
            {
                fr.hash_i = key % map._capacity;
            }

            // The count is now the new index.
            ice::u32 const index = map._count;

            // Set the key we are use to make the new entry.
            map._entries[index].key = key;

            // ... and the next to the previous index stored in the hashes
            map._entries[index].next = map._hashes[fr.hash_i];

            // ... then this is the first entry on this hash table.
            map._hashes[fr.hash_i] = index;

            // ... increase the entry count
            map._count += 1;

            return index;
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline void find_and_erase(ContainerT& map, ice::u64 key) noexcept
        {
            FindResult const fr = ice::detail::hashmap::find(map, key);
            if (fr.entry_i != Constant_EndOfList)
            {
                ice::detail::hashmap::erase(map, fr);
            }
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline auto find(ContainerT& map, typename ContainerT::ConstIterator it) noexcept -> FindResult
        {
            FindResult fr{
                .hash_i = Constant_EndOfList,
                .entry_prev = Constant_EndOfList,
                .entry_i = Constant_EndOfList,
            };

            if (map._count == 0)
            {
                return fr;
            }

            fr.hash_i = it._entry->key % map._capacity;
            fr.entry_i = map._hashes[fr.hash_i];

            while (fr.entry_i != Constant_EndOfList)
            {
                if ((map._entries + fr.entry_i) == it._entry)
                {
                    return fr;
                }

                fr.entry_prev = fr.entry_i;
                fr.entry_i = map._entries[fr.entry_i].next;
            }
            return fr;
        }

    } // namespace hashmap::detail

} // namespace ice
