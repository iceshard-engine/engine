/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container/contiguous_container.hxx>

#include <ice/container_logic.hxx>
#include <ice/string_types.hxx>
#include <ice/span.hxx>
#include <array>
#include <ice/array.hxx>

namespace ice
{

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
        ice::u32 _capacity;
        ice::u32 _count;
        ice::u32 _offset;
        Type* _data;

        inline explicit Queue(ice::Allocator& alloc) noexcept;
        inline Queue(Queue&& other) noexcept;
        inline Queue(Queue const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Queue() noexcept;

        inline auto operator=(Queue&& other) noexcept -> Queue&;
        inline auto operator=(Queue const& other) noexcept -> Queue&
            requires std::copy_constructible<Type>;

        auto operator[](ice::u32 idx) noexcept -> Type&;
        auto operator[](ice::u32 idx) const noexcept -> Type const&;
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
            ice::u32 next;
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
        ice::u32 _capacity;
        ice::u32 _count;

        ice::u32* _hashes;
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

        ice::u32 _capacity;
        ice::u32 _count;

        ice::u32 const* _hashes;
        Entry const* _entries;
        Type const* _data;
    };

    //! \brief A concept used to enable access to read-only operations for all compatible types.
    template<typename Type>
    concept HashMapReadAccess = requires(Type t) {
        { typename Type::Entry() } -> std::convertible_to<typename Type::Entry>;
        { t._capacity } -> std::convertible_to<ice::u32>;
        { t._count } -> std::convertible_to<ice::u32>;
        { t._hashes } -> std::convertible_to<ice::u32 const*>;
        { t._entries } -> std::convertible_to<typename Type::Entry const*>;
        { t._data } -> std::convertible_to<typename Type::ValueType const*>;
    };


    // TODO: Introduce our own type and create proper concepts for function access.
    template<typename T, ice::u32 Size>
    using StaticArray = std::array<T, Size>;

} // namespace ice
