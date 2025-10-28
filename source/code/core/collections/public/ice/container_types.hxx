/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container_logic.hxx>
#include <ice/span.hxx>
#include <array>

namespace ice
{

    //! \brief A simple contaier storing items in contignous memory.
    //!
    //! \details Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value cab be set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct Array
    {
        static_assert(
            Logic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        using ValueType = Type;
        using Iterator = Type*;
        using ReverSeIterator = std::reverse_iterator<Type*>;
        using ConstIterator = Type const*;
        using ConstReverseIterator = std::reverse_iterator<Type const*>;

        ice::Allocator* _allocator;
        ice::ucount _capacity;
        ice::ucount _count;
        Type* _data;

        inline explicit Array(ice::Allocator& alloc) noexcept;
        inline Array(Array&& other) noexcept;
        inline Array(Array const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Array() noexcept;

        inline Array(
            ice::Allocator& alloc,
            ice::Span<Type const> values
        ) noexcept requires std::copy_constructible<Type>;

        inline auto operator=(Array&& other) noexcept -> Array&;
        inline auto operator=(Array const& other) noexcept -> Array&
            requires std::copy_constructible<Type>;

        inline auto operator[](ice::ucount idx) noexcept -> Type&;
        inline auto operator[](ice::ucount idx) const noexcept -> Type const&;

        inline operator ice::Span<Type>() noexcept;
        inline operator ice::Span<Type const>() const noexcept;
    };


    //! \brief A double ended queue build on a circular buffer.
    //!
    //! \details Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct Queue
    {
        static_assert(
            Logic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        using ValueType = Type;

        ice::Allocator* _allocator;
        ice::ucount _capacity;
        ice::ucount _count;
        ice::ucount _offset;
        Type* _data;

        inline explicit Queue(ice::Allocator& alloc) noexcept;
        inline Queue(Queue&& other) noexcept;
        inline Queue(Queue const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Queue() noexcept;

        inline auto operator=(Queue&& other) noexcept -> Queue&;
        inline auto operator=(Queue const& other) noexcept -> Queue&
            requires std::copy_constructible<Type>;

        auto operator[](ice::ucount idx) noexcept -> Type&;
        auto operator[](ice::ucount idx) const noexcept -> Type const&;
    };


    //! \brief A hash map build on a single block of memory.
    //!
    //! \details Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct HashMap
    {
        static_assert(
            Logic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        using ValueType = Type;

        struct Entry
        {
            ice::u64 key;
            ice::ucount next;
        };

        struct ConstIterator
        {
            Entry const* _entry;
            Type const* _value;

            constexpr ConstIterator(std::nullptr_t) noexcept
                : _entry{ nullptr }
                , _value{ nullptr }
            {
            }

            constexpr ConstIterator(Entry const* entry, Type const* value) noexcept
                : _entry{ entry }
                , _value{ value }
            {
            }

            constexpr auto key() const noexcept -> ice::u64 const& { return _entry->key; }
            constexpr auto value() const noexcept -> Type const& { return *_value; }

            constexpr auto operator==(ConstIterator const& other) const noexcept { return _entry == other._entry; }
            constexpr auto operator!=(ConstIterator const& other) const noexcept { return !(*this == other); }

            constexpr void operator++() noexcept { _entry += 1; _value += 1; }
            constexpr auto operator*() const noexcept -> Type const& { return value(); }
        };

        ice::Allocator* _allocator;
        ice::ucount _capacity;
        ice::ucount _count;

        ice::ucount* _hashes;
        Entry* _entries;
        Type* _data;

        inline explicit HashMap(ice::Allocator& alloc) noexcept;
        inline HashMap(HashMap&& other) noexcept;
        inline HashMap(HashMap const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~HashMap() noexcept;

        inline auto operator=(HashMap&& other) noexcept -> HashMap&;
        inline auto operator=(HashMap const& other) noexcept -> HashMap&
            requires std::copy_constructible<Type>;
    };


    //! \brief A view into data created by a hashmap object.
    //!
    //! \note No modification of data is allowed through this type.
    //! \note Because this is only a view into a hashmap there is no difference in 'logic' so it's unused.
    template<typename Type, ice::ContainerLogic = ice::ContainerLogic::Trivial>
    struct HashMapView
    {
        using Entry = typename ice::HashMap<Type>::Entry;
        using ValueType = Type;

        ice::ucount _capacity;
        ice::ucount _count;

        ice::ucount const* _hashes;
        Entry const* _entries;
        Type const* _data;
    };

    //! \brief A concept used to enable access to read-only operations for all compatible types.
    template<typename Type>
    concept HashMapReadAccess = requires(Type t) {
        { typename Type::Entry() } -> std::convertible_to<typename Type::Entry>;
        { t._capacity } -> std::convertible_to<ice::ucount>;
        { t._count } -> std::convertible_to<ice::ucount>;
        { t._hashes } -> std::convertible_to<ice::ucount const*>;
        { t._entries } -> std::convertible_to<typename Type::Entry const*>;
        { t._data } -> std::convertible_to<typename Type::ValueType const*>;
    };


    // TODO: Introduce our own type and create proper concepts for function access.
    template<typename T, ice::u32 Size>
    using StaticArray = std::array<T, Size>;

} // namespace ice
