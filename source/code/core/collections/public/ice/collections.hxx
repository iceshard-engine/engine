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

    //! \brief A simple dynamic array object.
    //!
    //! \detail Manages a memory block big enough to hold the items that it holds.
    //! \note Used as the base building block in almost all other containts.
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
        inline Array(ice::Array<Type, Logic>&& other) noexcept;
        inline Array(ice::Array<Type, Logic> const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Array() noexcept;

        inline Array(
            ice::Allocator& alloc,
            ice::Span<Type const> values
        ) noexcept requires std::copy_constructible<Type>;

        inline auto operator=(ice::Array<Type, Logic>&& other) noexcept -> ice::Array<Type, Logic>&;
        inline auto operator=(ice::Array<Type, Logic> const& other) noexcept -> ice::Array<Type, Logic>&
            requires std::copy_constructible<Type>;

        inline auto operator[](ice::ucount idx) noexcept -> Type&;
        inline auto operator[](ice::ucount idx) const noexcept -> Type const&;

        inline operator ice::Span<Type>() noexcept;
        inline operator ice::Span<Type const>() const noexcept;
    };

} // namespace ice
