/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    namespace hashmap::detail
    {

        static constexpr ice::f32 Constant_HashMapMaxFill = 0.7f;

        struct FindResult;

        constexpr auto calc_storage_capacity(ice::ucount max_count) noexcept -> ice::ucount;

        template<typename Type, ice::ContainerLogic Logic>
        inline bool can_store_count(ice::HashMap<Type, Logic> const& map, ice::ucount expected_count) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto find(HashMapType const& map, ice::u64 key) noexcept -> FindResult;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto make(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> ice::ucount;

        template<typename Type, ice::ContainerLogic Logic>
        inline void erase(ice::HashMap<Type, Logic>& map, FindResult const fr) noexcept;

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto find_or_fail(HashMapType const& map, ice::u64 key) noexcept -> ice::ucount;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_or_make(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> ice::ucount;

        template<typename Type, ice::ContainerLogic Logic>
        inline void find_and_erase(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void rehash(ice::HashMap<Type, Logic>& map, ice::ucount new_capacity) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator it) noexcept -> FindResult;

        template<typename Type, ice::ContainerLogic Logic>
        inline void grow(ice::HashMap<Type, Logic>& map) noexcept;

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear_and_dealloc(ice::HashMap<Type, Logic>& map) noexcept;

    } // namespace hashmap::detail

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::HashMap(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _hashes{ nullptr }
        , _entries { nullptr }
        , _data{ nullptr }
    {
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::HashMap(HashMap&& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _count{ ice::exchange(other._count, 0) }
        , _hashes{ ice::exchange(other._hashes, nullptr) }
        , _entries{ ice::exchange(other._entries, nullptr) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::HashMap(HashMap const& other) noexcept
        requires std::copy_constructible<Type>
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _count{ 0 }
        , _hashes{ 0 }
        , _entries{ 0 }
        , _data{ 0 }
    {
        if (other._count > 0)
        {
            ice::hashmap::detail::rehash(*this, other._capacity);

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<Entry>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                Memory{ .location = _entries, .size = ice::size_of<Entry> * other._count, .alignment = ice::align_of<Entry> },
                Data{ .location = other._entries, .size = ice::size_of<Entry> * other._count, .alignment = ice::align_of<Entry> }
            );

            // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    Memory{
                        .location = _data,
                        .size = ice::size_of<Type> * other._count,
                        .alignment = ice::align_of<Type>
                    },
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    Memory{ .location = _data, .size = ice::size_of<Type> * other._count, .alignment = ice::align_of<Type> },
                    Data{ .location = other._data, .size = ice::size_of<Type> *other._count, .alignment = ice::align_of<Type> }
                );
            }

            ice::u32 idx = 0;
            for (Entry const& entry : ice::hashmap::entries(other._entries))
            {
                // First remember the previous set index...
                _entries[idx].next = _hashes[entry.key % _capacity];

                // ... then save the current index in the hashed array.
                _hashes[entry.key % _capacity] = idx;
                idx += 1;
            }

            _count = other._count;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::~HashMap() noexcept
    {
        ice::hashmap::detail::clear_and_dealloc(*this);
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto HashMap<Type, Logic>::operator=(HashMap&& other) noexcept -> HashMap&
    {
        if (this != &other)
        {
            ice::hashmap::detail::clear_and_dealloc(*this); // Clears the current data

            _allocator = other._allocator;
            _capacity = std::exchange(other._capacity, 0);
            _count = std::exchange(other._count, 0);
            _hashes = std::exchange(other._hashes, nullptr);
            _entries = std::exchange(other._entries, nullptr);
            _data = std::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto HashMap<Type, Logic>::operator=(HashMap const& other) noexcept -> HashMap&
        requires std::copy_constructible<Type>
    {
        if (this != &other)
        {
            ice::hashmap::clear(*this);

            // Grow if needed to the specific size
            if (ice::hashmap::detail::can_store_count(*this, other._count) == false)
            {
                ice::hashmap::detail::rehash(*this, ice::hashmap::detail::calc_storage_capacity(other._count));
            }

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<Entry>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                Memory{ .location = _entries, .size = ice::size_of<Entry> * other._count, .alignment = ice::align_of<Entry> },
                Data{ .location = other._entries, .size = ice::size_of<Entry> * other._count, .alignment = ice::align_of<Entry> }
            );

            // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    Memory{
                        .location = _data,
                        .size = ice::size_of<Type> * other._count,
                        .alignment = ice::align_of<Type>
                    },
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    Memory{ .location = _data, .size = ice::size_of<Type> * other._count, .alignment = ice::align_of<Type> },
                    Data{ .location = other._data, .size = ice::size_of<Type> * other._count, .alignment = ice::align_of<Type> }
                );
            }

            ice::u32 idx = 0;
            for (Entry const& entry : ice::hashmap::entries(other._entries))
            {
                // First remember the previous set index...
                _entries[idx].next = _hashes[entry.key % _capacity];

                // ... then save the current index in the hashed array.
                _hashes[entry.key % _capacity] = idx;
                idx += 1;
            }

            _count = other._count;
        }
        return this;
    }

    namespace hashmap::detail
    {

        static constexpr ice::u32 Constant_EndOfList = 0xffffffffu;

        struct FindResult
        {
            ice::ucount hash_i;
            ice::ucount entry_prev;
            ice::ucount entry_i;
        };


        constexpr auto calc_storage_capacity(ice::ucount max_count) noexcept -> ice::ucount
        {
            return ice::ucount(max_count / Constant_HashMapMaxFill + 0.99f /* magic */);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline bool can_store_count(ice::HashMap<Type, Logic> const& map, ice::ucount expected_count) noexcept
        {
            ice::ucount const max_ucount = ice::ucount(map._capacity * Constant_HashMapMaxFill);
            return max_ucount >= expected_count;
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto find(HashMapType const& map, ice::u64 key) noexcept -> FindResult
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

        template<typename Type, ice::ContainerLogic Logic>
        inline auto make(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> ice::ucount
        {
            FindResult fr = ice::hashmap::detail::find(map, key);
            if (fr.hash_i == Constant_EndOfList)
            {
                fr.hash_i = key % map._capacity;
            }

            // The count is now the new index.
            ice::ucount const index = map._count;

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

        template<typename Type, ice::ContainerLogic Logic>
        inline void erase(ice::HashMap<Type, Logic>& map, FindResult const fr) noexcept
        {
            using Entry = typename ice::HashMap<Type, Logic>::Entry;

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
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_at(map._data + fr.entry_i);
            }

            map._count -= 1;
            if (fr.entry_i == map._count)
            {
                return;
            }

            if constexpr (Logic == ContainerLogic::Complex)
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
            FindResult const last_key_first_entry = ice::hashmap::detail::find(map, map._entries[map._count].key);
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

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto find_or_fail(HashMapType const& map, ice::u64 key) noexcept -> ice::ucount
        {
            return ice::hashmap::detail::find(map, key).entry_i;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_or_make(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept -> ice::ucount
        {
            FindResult fr = ice::hashmap::detail::find(map, key);

            // If entry index is not valid we still might have a previous element on the same hash index.
            if (fr.entry_i != Constant_EndOfList)
            {
                return fr.entry_i;
            }

            if (fr.hash_i == Constant_EndOfList)
            {
                fr.hash_i = key % map._capacity;
            }

            // The count is now the new index.
            ice::ucount const index = map._count;

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

        template<typename Type, ice::ContainerLogic Logic>
        inline void find_and_erase(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept
        {
            FindResult const fr = ice::hashmap::detail::find(map, key);
            if (fr.entry_i != Constant_EndOfList)
            {
                ice::hashmap::detail::erase(map, fr);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void rehash(ice::HashMap<Type, Logic>& map, ice::ucount new_capacity) noexcept
        {
            using Entry = typename ice::HashMap<Type, Logic>::Entry;

            ICE_ASSERT_CORE(new_capacity * Constant_HashMapMaxFill >= map._count);

            ice::ucount* new_hashes_ptr = nullptr;
            Entry* new_entries_ptr = nullptr;
            Type* new_value_ptr = nullptr;

            if (new_capacity > 0)
            {
                ice::ucount const new_capacity_values = ice::ucount(new_capacity * Constant_HashMapMaxFill);

                ice::meminfo alloc_info = ice::meminfo_of<ice::u32> * new_capacity;
                ice::usize const offset_entries = alloc_info += ice::meminfo_of<Entry> * new_capacity_values;
                ice::usize const offset_values = alloc_info += ice::meminfo_of<Type> * new_capacity_values;

                ice::AllocResult const new_data = map._allocator->allocate(alloc_info);
                new_hashes_ptr = reinterpret_cast<ice::ucount*>(new_data.memory);
                new_entries_ptr = reinterpret_cast<Entry*>(ice::ptr_add(new_data.memory, offset_entries));
                new_value_ptr = reinterpret_cast<Type*>(ice::ptr_add(new_data.memory, offset_values));

                // Prepare hashes memory
                // TODO: memset?
                for (ice::ucount& hashed_idx : ice::Span<ice::ucount>{ new_hashes_ptr, new_capacity })
                {
                    hashed_idx = Constant_EndOfList;
                }

                if (map._count > 0)
                {
                    // NOTE: We keep the original entry + data indices, they don't need to change.

                    // Copy all the entries, this is always a POD type.
                    static_assert(std::is_pod_v<Entry>, "HashMap::Entry should not be changed!");
                    ice::memcpy(
                        Memory{ .location = new_entries_ptr, .size = ice::size_of<Entry> * map._count, .alignment = ice::align_of<Entry> },
                        Data{ .location = map._entries, .size = ice::size_of<Entry> * map._count, .alignment = ice::align_of<Entry> }
                    );

                    // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
                    if constexpr (Logic == ContainerLogic::Complex)
                    {
                        ice::mem_move_construct_n_at(
                            Memory{ .location = new_value_ptr, .size = ice::size_of<Type> * map._count, .alignment = ice::align_of<Type> },
                            map._data,
                            map._count
                        );
                    }
                    else
                    {
                        ice::memcpy(
                            Memory{ .location = new_value_ptr, .size = ice::size_of<Type> * map._count, .alignment = ice::align_of<Type> },
                            Data{ .location = map._data, .size = ice::size_of<Type> * map._count, .alignment = ice::align_of<Type> }
                        );
                    }

                    ice::u32 idx = 0;
                    for (Entry const& entry : ice::hashmap::entries(map))
                    {
                        // First remember the previous set index...
                        new_entries_ptr[idx].next = new_hashes_ptr[entry.key % new_capacity];

                        // ... then save the current index in the hashed array.
                        new_hashes_ptr[entry.key % new_capacity] = idx;
                        idx += 1;
                    }
                }
            }

            map._allocator->deallocate(ice::hashmap::memory(map));
            map._capacity = new_capacity;
            map._hashes = new_hashes_ptr;
            map._entries = new_entries_ptr;
            map._data = new_value_ptr;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find(ice::HashMap<Type, Logic>& map, typename ice::HashMap<Type, Logic>::ConstIterator it) noexcept -> FindResult
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

            fr.hash_i = it->entry->key % map._capacity;
            fr.entry_i = map._hashes[fr.hash_i];

            while (fr.entry_i != Constant_EndOfList)
            {
                if ((map._data + fr.entry_i) == it->entry)
                {
                    return fr;
                }

                fr.entry_prev = fr.entry_i;
                fr.entry_i = map._data[fr.entry_i].next;
            }
            return fr;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void grow(ice::HashMap<Type, Logic>& map) noexcept
        {
            ice::hashmap::detail::rehash(map, map._capacity * 2 + 8);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear_and_dealloc(ice::HashMap<Type, Logic>& map) noexcept
        {
            ice::hashmap::clear(map);
            ice::hashmap::detail::rehash(map, 0);
        }

    } // namespace hashmap::detail

    namespace hashmap
    {

        //! \brief Allocates enough space in the hash map to hold the given amount of values.
        //!
        //! \note Keep in mind, the number of valies a hashmap can store is lower than it's total capacity.
        template<typename Type, ice::ContainerLogic Logic>
        inline void reserve(ice::HashMap<Type, Logic>& map, ice::ucount new_count) noexcept
        {
            ice::hashmap::detail::rehash(map, ice::hashmap::detail::calc_storage_capacity(new_count));
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear(ice::HashMap<Type, Logic>& map) noexcept
        {
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_n_at(map._data, map._count);
            }

            map._count = 0;
            for (ice::ucount hash_idx = 0; hash_idx < map._capacity; ++hash_idx)
            {
                // TODO: memset?
                map._hashes[hash_idx] = ice::hashmap::detail::Constant_EndOfList;
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void shrink(ice::HashMap<Type, Logic>& map) noexcept
        {
            ice::hashmap::detail::rehash(map, ice::hashmap::detail::calc_storage_capacity(map._count));
        }

        template<typename Type, ice::ContainerLogic Logic, typename Value>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Value const& value) noexcept
        {
            if (ice::hashmap::full(map))
            {
                ice::hashmap::detail::grow(map);
            }

            ice::ucount const index = ice::hashmap::detail::find_or_make(map, key);
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
            requires std::move_constructible<Type>
        inline void set(ice::HashMap<Type, Logic>& map, ice::u64 key, Type&& value) noexcept
        {
            if (ice::hashmap::full(map))
            {
                ice::hashmap::detail::grow(map);
            }

            ice::ucount const index = ice::hashmap::detail::find_or_make(map, key);
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
        inline void remove(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept
        {
            ice::hashmap::detail::find_and_erase(map, key);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto values(ice::HashMap<Type, Logic>& map) noexcept -> ice::Span<Type>
        {
            return ice::Span{ map._data, map._count };
        }


        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool full(HashMapType const& map) noexcept
        {
            ice::ucount const max_count = ice::ucount(map._capacity * ice::hashmap::detail::Constant_HashMapMaxFill);
            ICE_ASSERT_CORE(max_count >= map._count);

            return max_count == map._count;
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool empty(HashMapType const& map) noexcept
        {
            return map._count == 0;
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline bool has(HashMapType const& map, ice::u64 key) noexcept
        {
            return ice::hashmap::detail::find_or_fail(map, key) != ice::hashmap::detail::Constant_EndOfList;
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto get(HashMapType const& map, ice::u64 key, typename HashMapType::ValueType const& fallback_value) noexcept -> typename HashMapType::ValueType const&
        {
            ice::ucount const index = ice::hashmap::detail::find_or_fail(map, key);
            return index == ice::hashmap::detail::Constant_EndOfList
                ? fallback_value
                : map._data[index];
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto get(HashMapType const& map, ice::u64 key, std::nullptr_t) noexcept -> typename HashMapType::ValueType
        {
            ice::ucount const index = ice::hashmap::detail::find_or_fail(map, key);
            return index == ice::hashmap::detail::Constant_EndOfList
                ? nullptr
                : map._data[index];
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto try_get(HashMapType const& map, ice::u64 key) noexcept -> typename HashMapType::ValueType const*
        {
            ice::ucount const index = ice::hashmap::detail::find_or_fail(map, key);
            return index == ice::hashmap::detail::Constant_EndOfList
                ? nullptr
                : map._data + index;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto begin(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator
        {
            return { map._entries, map._data };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto end(ice::HashMap<Type, Logic> const& map) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator
        {
            return { map._entries + map._count, map._data + map._count };
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto values(HashMapType const& map) noexcept -> ice::Span<typename HashMapType::ValueType const>
        {
            return ice::Span{ map._data, map._count };
        }

        template<typename HashMapType> requires HashMapReadAccess<HashMapType>
        inline auto entries(HashMapType const& map) noexcept -> ice::Span<typename HashMapType::Entry const>
        {
            return ice::Span{ map._entries, map._count };
        }


        template<typename Type, ice::ContainerLogic Logic>
        inline auto memory(ice::HashMap<Type, Logic>& map) noexcept -> ice::Memory
        {
            ice::ucount const capacity_values = ice::ucount(map._capacity * ice::hashmap::detail::Constant_HashMapMaxFill);

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
        inline void insert(ice::HashMap<Type, Logic>& map, ice::u64 key, Type const& value) noexcept
        {
            if (ice::hashmap::full(map))
            {
                ice::hashmap::detail::grow(map);
            }

            ice::ucount const index = ice::hashmap::detail::make(map, key);
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
            ice::hashmap::detail::FindResult const fr = ice::hashmap::detail::find(map, it);
            if (fr.entry_i != ice::hashmap::detail::Constant_EndOfList)
            {
                ice::hashmap::detail::erase(map, fr);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void remove_all(ice::HashMap<Type, Logic>& map, ice::u64 key) noexcept
        {
            while (ice::hashmap::has(map, key))
            {
                ice::hashmap::remove(map, key);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto count(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> ice::ucount
        {
            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;

            ice::ucount result = 0;
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
                ice::array::push_back(items, *it._value);
                it = ice::multi_hashmap::find_next(map, it);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto find_first(ice::HashMap<Type, Logic> const& map, ice::u64 key) noexcept -> typename ice::HashMap<Type, Logic>::ConstIterator
        {
            using ConstIterator = typename ice::HashMap<Type, Logic>::ConstIterator;

            ice::ucount const index = ice::hashmap::detail::find_or_fail(map, key);
            if (index == ice::hashmap::detail::Constant_EndOfList)
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

            ice::ucount index = it._entry->next;
            while (index != ice::hashmap::detail::Constant_EndOfList)
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
