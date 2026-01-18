/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/hashmap_details.hxx>
#include <ice/container/associative_container.hxx>
#include <ice/container/resizable_container.hxx>

namespace ice
{

    //! \brief A Map container designed for storing values using hashed values.
    //! \note This implementation only allows `u64` keys but provides some utility to calculate hashes
    //!   from well-known engine types like \ref ice::StringID.
    //!
    //! \details Manages single block of memory big enough to hold all data.
    //!   The implementation seperates the storage of values and internal data
    //!   into different blocks allowing to access values separately as continous data.
    //!
    //! \tparam Logic The operation logic used during memory operations for the given type.
    //!   This value can be forced by the user for specific behavior requirements.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct HashMap
        : public ice::container::AssociativeContainer
        , public ice::container::ResizableContainer
    {
        static constexpr ContainerLogic OperationLogic = Logic;
        static_assert(
            OperationLogic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        struct ConstIterator;

        using KeyType = ice::u64;
        using ValueType = Type;
        using ConstContainerValueType = Type const;
        using Iterator = ConstIterator;
        using SizeType = ice::ncount;
        using ContainerTag = ice::concepts::ContiguousContainerTag;

        struct EntryType
        {
            KeyType key;
            ice::u32 next;
        };

        ice::Allocator* _allocator;
        ice::u32 _capacity;
        ice::u32 _count;

        ice::u32* _hashes;
        EntryType* _entries;
        ValueType* _data;

        inline explicit HashMap(ice::Allocator& alloc) noexcept;
        inline ~HashMap() noexcept;

        inline HashMap(HashMap&& other) noexcept;
        inline HashMap(HashMap const& other) noexcept
            requires std::copy_constructible<Type>;

        inline auto operator=(HashMap&& other) noexcept -> HashMap&;
        inline auto operator=(HashMap const& other) noexcept -> HashMap&
            requires std::copy_constructible<Type>;

        // API Requirements Of: AssociativeContainer
        constexpr auto size() const noexcept -> SizeType { return { _count, sizeof(ValueType) }; }
        template<typename Self>
        constexpr auto find(this Self&& self, KeyType key) noexcept -> ice::container::ValuePtr<Self>;

        // Additional functionality
        template<typename Self>
        constexpr auto values(this Self&& self) noexcept -> ice::container::SpanType<Self>;

        // API Requirements Of: ResizableContainer
        constexpr auto capacity() const noexcept -> SizeType { return { _capacity, sizeof(ValueType) }; }
        constexpr void set_capacity(ice::ncount new_capacity) noexcept;
        constexpr void clear() noexcept;

        // API Requirements Of: IterableContainer
        template<typename Self>
        constexpr auto begin(this Self&& self) noexcept -> ice::container::Iterator<Self>;
        template<typename Self>
        constexpr auto end(this Self&& self) noexcept -> ice::container::Iterator<Self>;

        // API Requriements Of: Memory and Data
        constexpr auto memory_view() noexcept -> ice::Memory;
        constexpr auto entries_memory_view() noexcept -> ice::Memory;

        // Data Helpers
        constexpr auto values_data_view() const noexcept -> ice::Data;
        constexpr auto entries_data_view() const noexcept -> ice::Data;
    };

    template<typename Type, ice::ContainerLogic Logic>
    struct HashMap<Type, Logic>::ConstIterator
    {
        EntryType const* _entry;
        ValueType const* _value;

        constexpr ConstIterator(std::nullptr_t) noexcept
            : _entry{ nullptr }
            , _value{ nullptr }
        { }

        constexpr ConstIterator(EntryType const* entry, Type const* value) noexcept
            : _entry{ entry }
            , _value{ value }
        { }

        constexpr auto key() const noexcept -> ice::u64 const& { return _entry->key; }
        constexpr auto value() const noexcept -> Type const& { return *_value; }

        constexpr auto operator==(ConstIterator const& other) const noexcept { return _entry == other._entry; }
        constexpr auto operator!=(ConstIterator const& other) const noexcept { return !(*this == other); }

        constexpr void operator++() noexcept { _entry += 1; _value += 1; }
        constexpr auto operator*() const noexcept -> Type const& { return value(); }
    };

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::HashMap(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _hashes{ nullptr }
        , _entries { nullptr }
        , _data{ nullptr }
    { }

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::~HashMap() noexcept
    {
        clear();
        set_capacity(0);
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline HashMap<Type, Logic>::HashMap(HashMap&& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _count{ ice::exchange(other._count, 0) }
        , _hashes{ ice::exchange(other._hashes, nullptr) }
        , _entries{ ice::exchange(other._entries, nullptr) }
        , _data{ ice::exchange(other._data, nullptr) }
    { }

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
            set_capacity(other.capacity());

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<EntryType>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                Memory{ .location = _entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> },
                Data{ .location = other._entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> }
            );

            // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
            ice::Memory const self_values_memory{ _data, this->size(), ice::align_of<ValueType> };
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    self_values_memory,
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    self_values_memory,
                    other.values_data_view()
                );
            }

            ice::u32 idx = 0;
            for (EntryType const& entry : ice::detail::hashmap::entries(*this))
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
    inline auto HashMap<Type, Logic>::operator=(HashMap&& other) noexcept -> HashMap&
    {
        if (this != &other)
        {
            clear();
            set_capacity(other.capacity());

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
            this->clear();

            // Grows the internal data structure to the required size.
            this->reserve(other.size());

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<EntryType>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                this->entries_memory_view(),
                other.entries_data_view()
            );

            // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
            ice::Memory const self_values_memory{ _data, this->size(), ice::align_of<ValueType> };
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    self_values_memory,
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    self_values_memory,
                    other.values_data_view()
                );
            }

            ice::u32 idx = 0;
            for (EntryType const& entry : ice::detail::hashmap::entries(*this))
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

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto ice::HashMap<Type, Logic>::find(
        this Self&& self,
        KeyType key
    ) noexcept -> ice::container::ValuePtr<Self>
    {
        ice::u32 const entry_index = ice::detail::hashmap::find_or_fail(self, key);
        return entry_index != ice::detail::hashmap::Constant_EndOfList
            ? ice::addressof(self._data[entry_index])
            : nullptr;
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto ice::HashMap<Type, Logic>::values(this Self&& self) noexcept -> ice::container::SpanType<Self>
    {
        return { self._data, self._count };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::HashMap<Type, Logic>::set_capacity(ice::ncount new_capacity) noexcept
    {
        ice::u32* new_hashes_ptr = nullptr;
        EntryType* new_entries_ptr = nullptr;
        ValueType* new_value_ptr = nullptr;

        if (new_capacity > 0)
        {
            ICE_ASSERT_CORE(new_capacity >= this->size()); // We don't support support reduction below current item count!
            ice::ncount const new_internal_capacity = ice::detail::hashmap::capacity_with_overhead(new_capacity);

            ice::ChunkedAllocRequest alloc_reqest;
            alloc_reqest.include(new_hashes_ptr, new_internal_capacity);
            alloc_reqest.include(new_entries_ptr, new_capacity);
            alloc_reqest.include(new_value_ptr, new_capacity);
            _allocator->allocate(alloc_reqest);

            // Prepare hashes memory
            std::memset(new_hashes_ptr, 0xffffffff, new_internal_capacity * sizeof(u32));

            if (_count > 0)
            {
                // NOTE: We keep the original entry + data indices, they don't need to change.

                // Copy all the entries, this is always a POD type.
                static_assert(std::is_pod_v<EntryType>, "HashMap::EntryType should not be changed!");
                std::memcpy(new_entries_ptr, this->_entries, sizeof(EntryType) * _count);

                // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
                ice::Memory const new_values_memory{ new_value_ptr, this->size(), ice::align_of<ValueType> };
                if constexpr (OperationLogic == ContainerLogic::Complex)
                {
                    ice::mem_move_construct_n_at(new_values_memory, _data, _count);
                }
                else
                {
                    ice::memcpy(new_values_memory, this->values_data_view());
                }

                ice::u32 idx = 0;
                ice::u32 const new_capacity_u32 = new_capacity.u32();
                for (EntryType const& entry : ice::detail::hashmap::entries(*this))
                {
                    // First remember the previous set index...
                    new_entries_ptr[idx].next = new_hashes_ptr[entry.key % new_capacity_u32];

                    // ... then save the current index in the hashed array.
                    new_hashes_ptr[entry.key % new_capacity_u32] = idx;
                    idx += 1;
                }
            }
        }

        _allocator->deallocate(this->memory_view());
        _capacity = new_capacity.u32();
        _hashes = new_hashes_ptr;
        _entries = new_entries_ptr;
        _data = new_value_ptr;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::HashMap<Type, Logic>::clear() noexcept
    {
        if (_count == 0)
        {
            return;
        }

        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_destruct_n_at(_data, _count);
        }

        ice::ncount const internal_capacity = ice::detail::hashmap::capacity_with_overhead(_capacity);
        std::memset(_hashes, ice::detail::hashmap::Constant_EndOfList, sizeof(u32) * internal_capacity);

        _count = 0;
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto HashMap<Type, Logic>::begin(this Self&& self) noexcept -> ice::container::Iterator<Self>
    {
        return { self._entries, self._data };
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto HashMap<Type, Logic>::end(this Self&& self) noexcept -> ice::container::Iterator<Self>
    {
        return { self._entries + self._count, self._data + self._count };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto ice::HashMap<Type, Logic>::memory_view() noexcept -> ice::Memory
    {
        ice::meminfo const info = ice::detail::hashmap::calc_meminfo<EntryType, ValueType>(_capacity);
        return ice::Memory{
            .location = _hashes,
            .size = info.size,
            .alignment = ice::align_of<ValueType>
        };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto ice::HashMap<Type, Logic>::entries_memory_view() noexcept -> ice::Memory
    {
        return ice::Memory{
            .location = _entries,
            .size = ice::size_of<EntryType> * _count,
            .alignment = ice::align_of<EntryType>
        };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto ice::HashMap<Type, Logic>::values_data_view() const noexcept -> ice::Data
    {
        return Data{
            .location = _data,
            .size = this->size(),
            .alignment = ice::align_of<ValueType>
        };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto ice::HashMap<Type, Logic>::entries_data_view() const noexcept -> ice::Data
    {
        return Data{
            .location = _entries,
            .size = ice::size_of<EntryType> * _count,
            .alignment = ice::align_of<EntryType>
        };
    }

} // namespace ice
