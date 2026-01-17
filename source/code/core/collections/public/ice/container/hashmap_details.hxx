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

        constexpr auto calc_required_capacity(ice::ncount max_count) noexcept -> ice::ncount
        {
            return static_cast<ice::ncount::base_type>(
                max_count.native() / Constant_HashMapMaxFill + 0.99f /* magic */
            );
        }

        constexpr bool can_store_expected_size(ice::ncount capacity, ice::ncount expected_size) noexcept
        {
            ice::ncount const max_size_for_capacity = static_cast<ice::ncount::base_type>(
                capacity * Constant_HashMapMaxFill
            );
            return max_size_for_capacity >= expected_size;
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
        inline auto find_or_fail(ContainerT const& map, ice::u64 key) noexcept -> ice::u32
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
        inline void rehash(ContainerT& map, ice::u32 new_capacity) noexcept
        {
            using Entry = typename ContainerT::EntryType;
            using Type = typename ContainerT::ValueType;

            ICE_ASSERT_CORE(new_capacity * Constant_HashMapMaxFill >= map._count);

            ice::u32* new_hashes_ptr = nullptr;
            Entry* new_entries_ptr = nullptr;
            Type* new_value_ptr = nullptr;

            if (new_capacity > 0)
            {
                ice::u32 const new_capacity_values = ice::u32(new_capacity * Constant_HashMapMaxFill);

                ice::meminfo alloc_info = ice::meminfo_of<ice::u32> *new_capacity;
                ice::usize const offset_entries = alloc_info += ice::meminfo_of<Entry> *new_capacity_values;
                ice::usize const offset_values = alloc_info += ice::meminfo_of<Type> *new_capacity_values;

                ice::AllocResult const new_data = map._allocator->allocate(alloc_info);
                new_hashes_ptr = reinterpret_cast<ice::u32*>(new_data.memory);
                new_entries_ptr = reinterpret_cast<Entry*>(ice::ptr_add(new_data.memory, offset_entries));
                new_value_ptr = reinterpret_cast<Type*>(ice::ptr_add(new_data.memory, offset_values));

                // Prepare hashes memory
                // TODO: memset?
                for (ice::u32& hashed_idx : ice::Span<ice::u32>{ new_hashes_ptr, new_capacity })
                {
                    hashed_idx = Constant_EndOfList;
                }

                if (map._count > 0)
                {
                    // NOTE: We keep the original entry + data indices, they don't need to change.

                    // Copy all the entries, this is always a POD type.
                    static_assert(std::is_pod_v<Entry>, "HashMap::Entry should not be changed!");
                    ice::memcpy(
                        Memory{ .location = new_entries_ptr, .size = ice::size_of<Entry> *map._count, .alignment = ice::align_of<Entry> },
                        Data{ .location = map._entries, .size = ice::size_of<Entry> *map._count, .alignment = ice::align_of<Entry> }
                    );

                    // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
                    if constexpr (ContainerT::OperationLogic == ContainerLogic::Complex)
                    {
                        ice::mem_move_construct_n_at(
                            Memory{ .location = new_value_ptr, .size = ice::size_of<Type> *map._count, .alignment = ice::align_of<Type> },
                            map._data,
                            map._count
                        );
                    }
                    else
                    {
                        ice::memcpy(
                            Memory{ .location = new_value_ptr, .size = ice::size_of<Type> *map._count, .alignment = ice::align_of<Type> },
                            Data{ .location = map._data, .size = ice::size_of<Type> *map._count, .alignment = ice::align_of<Type> }
                        );
                    }

                    ICE_ASSERT_CORE(false);
                    //ice::u32 idx = 0;
                    //for (Entry const& entry : ice::hashmap::entries(map))
                    //{
                    //    // First remember the previous set index...
                    //    new_entries_ptr[idx].next = new_hashes_ptr[entry.key % new_capacity];

                    //    // ... then save the current index in the hashed array.
                    //    new_hashes_ptr[entry.key % new_capacity] = idx;
                    //    idx += 1;
                    //}
                }
            }

            ICE_ASSERT_CORE(false);
            //map._allocator->deallocate(ice::hashmap::memory(map));
            map._capacity = new_capacity;
            map._hashes = new_hashes_ptr;
            map._entries = new_entries_ptr;
            map._data = new_value_ptr;
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

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline void grow(ContainerT& map) noexcept
        {
            ice::detail::hashmap::rehash(map, map._capacity * 2 + 8);
        }

        template<ice::detail::hashmap::HashMapContainer ContainerT>
        inline void clear_and_dealloc(ContainerT& map) noexcept
        {
            ICE_ASSERT_CORE(false);
            //ice::hashmap::clear(map);
            ice::detail::hashmap::rehash(map, 0);
        }

    } // namespace hashmap::detail

} // namespace ice
