#pragma once
#include <ice/container/hashmap_details.hxx>
#include <ice/container/associative_container.hxx>

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
    {
        static constexpr ContainerLogic OperationLogic = Logic;
        static_assert(
            OperationLogic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        struct ConstIterator;

        using ValueType = Type;
        using ConstContainerValueType = Type const;
        using Iterator = ConstIterator;
        using SizeType = ice::ncount;
        using ContainerTag = ice::concepts::ContiguousContainerTag;

        struct EntryType
        {
            ice::u64 key;
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
        constexpr auto size() const noexcept -> SizeType { return _count; }
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
        ice::detail::hashmap::clear_and_dealloc(*this);
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
            ice::detail::hashmap::rehash(*this, other._capacity);

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<EntryType>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                Memory{ .location = _entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> },
                Data{ .location = other._entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> }
            );

            // If the value is a complex type, properly move construct it in the new location + destroy in the old one.
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    Memory{
                        .location = _data,
                        .size = ice::size_of<ValueType> * other._count,
                        .alignment = ice::align_of<ValueType>
                    },
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    Memory{ .location = _data, .size = ice::size_of<ValueType> * other._count, .alignment = ice::align_of<Type> },
                    Data{ .location = other._data, .size = ice::size_of<ValueType> *other._count, .alignment = ice::align_of<Type> }
                );
            }

            ice::u32 idx = 0;
            ICE_ASSERT_CORE(false);
            //for (EntryType const& entry : ice::hashmap::entries(other._entries))
            //{
            //    // First remember the previous set index...
            //    _entries[idx].next = _hashes[entry.key % _capacity];

            //    // ... then save the current index in the hashed array.
            //    _hashes[entry.key % _capacity] = idx;
            //    idx += 1;
            //}

            _count = other._count;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto HashMap<Type, Logic>::operator=(HashMap&& other) noexcept -> HashMap&
    {
        if (this != &other)
        {
            ice::detail::hashmap::clear_and_dealloc(*this); // Clears the current data

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
            ICE_ASSERT_CORE(false);
            //ice::hashmap::clear(*this);

            // Grow if needed to the specific size
            if (ice::detail::hashmap::can_store_expected_size(*this, other._count) == false)
            {
                ice::detail::hashmap::rehash(*this, ice::detail::hashmap::calc_required_capacity(other._count));
            }

            // NOTE: We keep the original entry + data indices, they don't need to change.
            // Copy all the entries, this is always a POD type.
            static_assert(std::is_pod_v<EntryType>, "HashMap::Entry should not be changed!");
            ice::memcpy(
                Memory{ .location = _entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> },
                Data{ .location = other._entries, .size = ice::size_of<EntryType> * other._count, .alignment = ice::align_of<EntryType> }
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
            ICE_ASSERT_CORE(false);
            //for (EntryType const& entry : ice::hashmap::entries(other._entries))
            //{
            //    // First remember the previous set index...
            //    _entries[idx].next = _hashes[entry.key % _capacity];

            //    // ... then save the current index in the hashed array.
            //    _hashes[entry.key % _capacity] = idx;
            //    idx += 1;
            //}

            _count = other._count;
        }
        return this;
    }

} // namespace ice
