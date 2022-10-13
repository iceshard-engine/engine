#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/span.hxx>

namespace ice
{

    //! \brief The logic implemented by a collectiont type when working with data. (Copying, Moving, Removing, etc.)
    //! \note The picked logic will affect performance, but may also impose restrictions.
    enum class CollectionLogic
    {
        //! \brief The collection only handles plain old data and is allowed to memcopy values.
        //! \pre The type stored in the collection satisfies `std::is_pod`
        PlainOldData,

        //! \brief The collection handles complex data types and properly implements copy and move semantics.
        Complex,
    };

    //! \brief A simple contaier storing items in contignous memory.
    //!
    //! \detail Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::CollectionLogic Logic = CollectionLogic::PlainOldData>
    struct Array
    {
        static_assert(
            Logic != CollectionLogic::PlainOldData || std::is_pod_v<Type>,
            "Collection element type is not 'PlainOldData'!"
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
    //! \detail Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::CollectionLogic Logic = CollectionLogic::PlainOldData>
    struct Queue
    {
        static_assert(
            Logic != CollectionLogic::PlainOldData || std::is_pod_v<Type>,
            "Collection element type is not 'PlainOldData'!"
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
    //! \detail Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::CollectionLogic Logic = CollectionLogic::PlainOldData>
    struct HashMap
    {
        static_assert(
            Logic != CollectionLogic::PlainOldData || std::is_pod_v<Type>,
            "Collection element type is not 'PlainOldData'!"
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

            ConstIterator(std::nullptr_t) noexcept
                : _entry{ nullptr }
                , _value{ nullptr }
            {
            }

            ConstIterator(Entry const* entry, Type const* value) noexcept
                : _entry{ entry }
                , _value{ value }
            {
            }

            auto key() const noexcept -> ice::u64& { return _entry->key; }
            auto value() const noexcept -> Type const& { return *_value; }

            auto operator==(ConstIterator const& other) const noexcept { return _entry == other._entry; }
            auto operator!=(ConstIterator const& other) const noexcept { return !(*this == other); }

            void operator++() noexcept { _entry += 1; _value += 1; }
            auto operator*() const noexcept -> Type const& { return value(); }
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
    template<typename Type, ice::CollectionLogic = ice::CollectionLogic::PlainOldData>
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

} // namespace ice
