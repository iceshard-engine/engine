#pragma once
#include <ice/hashmap.hxx>

namespace ice
{

    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct MultiHashMap : public ice::HashMap<Type, Logic>
    {
        using HashMap<Type, Logic>::HashMap;

        using EntryType = typename HashMap<Type, Logic>::EntryType;
        using KeyType = typename HashMap<Type, Logic>::KeyType;
        using ValueType = typename HashMap<Type, Logic>::ValueType;
        using ConstContainerValueType = typename HashMap<Type, Logic>::ConstContainerValueType;
        using SizeType = typename HashMap<Type, Logic>::SizeType;
        using Iterator = typename HashMap<Type, Logic>::Iterator;
        using ConstIterator = typename HashMap<Type, Logic>::ConstIterator;

        struct ConstMultiIterator;

        // API Specific to this extended type of a HashMap
        template<ice::concepts::HashableKeyType HashableKeyT>
        constexpr auto count_values(HashableKeyT const& key) noexcept -> SizeType;

        template<typename Self, ice::concepts::HashableKeyType HashableKeyT>
        constexpr auto find_values(
            this Self const& self, HashableKeyT const& key
        ) noexcept -> ConstMultiIterator;

        template<
            typename Self,
            ice::concepts::HashableKeyType HashableKeyT,
            typename InValueType = typename std::remove_reference_t<Self>::ValueType
        >
            requires(std::convertible_to<InValueType, ice::container::ValueType<Self>>)
        constexpr auto insert(
            this Self&& self, HashableKeyT const& key, InValueType&& in_value
        ) noexcept -> ice::container::ValueRef<Self>;

        template<ice::concepts::HashableKeyType HashableKeyT>
        constexpr void remove_all(HashableKeyT const& key) noexcept;
    };

    template<typename Type, ice::ContainerLogic Logic>
    struct MultiHashMap<Type, Logic>::ConstMultiIterator
    {
        ice::u32 _current;
        EntryType const* _entries;
        ValueType const* _values;

        constexpr ConstMultiIterator(std::nullptr_t) noexcept
            : _current{ ice::detail::hashmap::Constant_EndOfList }
            , _entries{ nullptr }
            , _values{ nullptr }
        { }

        constexpr ConstMultiIterator(
            ice::u32 current_entry,
            EntryType const* entries,
            ValueType const* values
        ) noexcept
            : _current{ current_entry }
            , _entries{ entries }
            , _values{ values }
        { }

        constexpr bool has_next() const noexcept { return _current != ice::detail::hashmap::Constant_EndOfList; }
        constexpr void next() noexcept
        {
            ICE_ASSERT_CORE(has_next());

            _current = _entries[_current].next;
            if (_current == ice::detail::hashmap::Constant_EndOfList)
            {
                _entries = nullptr;
                _values = nullptr;
            }
        }

        constexpr auto key() const noexcept -> ice::u64 const& { return _entries[_current].key; }
        constexpr auto value() const noexcept -> Type const& { return _values[_current]; }

        constexpr auto operator==(ConstMultiIterator const& other) const noexcept
        {
            return _entries == other._entries && _current == other._current;
        }

        constexpr void operator++() noexcept { next(); }
        constexpr auto operator*() const noexcept -> Type const& { return value(); }
    };

    template<typename Type, ice::ContainerLogic Logic>
    template<ice::concepts::HashableKeyType HashableKeyT>
    inline constexpr auto MultiHashMap<Type, Logic>::count_values(
        HashableKeyT const& key
    ) noexcept -> SizeType
    {
        ice::u64 result = 0;

        auto it = find_values(key);
        while (it.has_next())
        {
            result += 1;
            it.next();
        }
        return { result, sizeof(ValueType) };
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self, ice::concepts::HashableKeyType HashableKeyT>
    inline constexpr auto MultiHashMap<Type, Logic>::find_values(
        this Self const& self, HashableKeyT const& key
    ) noexcept -> ConstMultiIterator
    {
        ice::u32 const index = ice::detail::hashmap::find_or_fail(self, ice::hash(key));
        if (index != ice::detail::hashmap::Constant_EndOfList)
        {
            return { index, self._entries, self._data };
        }
        return { nullptr };
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self, ice::concepts::HashableKeyType HashableKeyT, typename InValueType>
        requires(std::convertible_to<InValueType, ice::container::ValueType<Self>>)
    constexpr auto MultiHashMap<Type, Logic>::insert(
        this Self&& self, HashableKeyT const& key, InValueType&& in_value
    ) noexcept -> ice::container::ValueRef<Self>
    {
        if (self.is_full())
        {
            self.grow();
        }

        ice::u32 const index = ice::detail::hashmap::make(self, ice::hash(key));
        if constexpr (Logic == ContainerLogic::Complex)
        {
            // If the index is below the current map._count we need to destroy the previous value.
            if ((index + 1) < self._count)
            {
                ice::mem_destruct_at(self._data + index);
            }

            ice::mem_construct_at<ValueType>(
                self._data + index,
                ice::forward<InValueType>(in_value)
            );
        }
        else
        {
            self._data[index] = in_value;
        }

        return self._data[index];
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<ice::concepts::HashableKeyType HashableKeyT>
    inline constexpr void MultiHashMap<Type, Logic>::remove_all(HashableKeyT const& key) noexcept
    {
        ice::u64 const key_hash = ice::hash(key);
        while (this->find(key_hash) != nullptr)
        {
            this->remove(key_hash);
        }
    }

} // namespace ice
