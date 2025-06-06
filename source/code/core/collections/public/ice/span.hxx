/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/mem_data.hxx>
#include <ice/mem_memory.hxx>
#include <ice/container_logic.hxx>
#include <array> // TODO: Introduce our own static array object.

namespace ice
{

    //! \brief A view into an array of objects laid out in contiguous memory.
    template<typename Type>
    struct Span
    {
        using ValueType = Type;
        using Iterator = Type*;
        using ReverseIterator = std::reverse_iterator<Type*>;
        using ConstIterator = Type const*;
        using ConstReverseIterator = std::reverse_iterator<Type const*>;

        ice::ucount _count;
        Type* _data;

        constexpr Span() noexcept;
        constexpr Span(ice::Span<Type>&& other) noexcept = default;
        constexpr Span(ice::Span<Type> const& other) noexcept = default;
        constexpr Span(Type* location, ice::ucount count) noexcept;
        constexpr Span(Type* from, Type* to) noexcept;

        template<ice::ucount Size>
        constexpr Span(Type(&location)[Size]) noexcept;

        constexpr auto operator=(ice::Span<Type>&& other) noexcept -> ice::Span<Type>& = default;
        constexpr auto operator=(ice::Span<Type> const& other) noexcept -> ice::Span<Type>& = default;

        constexpr auto operator[](ice::ucount idx) const noexcept -> Type&;
        constexpr operator ice::Span<Type const>() noexcept { return { _data, _count }; }
        constexpr operator ice::Span<Type const>() const noexcept { return { _data, _count }; }
    };

    template<typename T, ice::ucount Size> Span(T(&)[Size]) noexcept -> Span<T>;
    template<typename T> Span(ice::Span<T>&&) noexcept -> Span<T>;
    template<typename T> Span(ice::Span<T> const&) noexcept -> Span<T>;
    template<typename T, ContainerLogic Logic, template<typename, ContainerLogic> typename Container> Span(Container<T, Logic> const&) noexcept -> Span<T>;

    namespace span
    {

        template<typename Type>
        constexpr bool empty(ice::Span<Type> span) noexcept;

        template<typename Type>
        constexpr bool any(ice::Span<Type> span) noexcept;

        template<typename Type>
        constexpr auto data(ice::Span<Type> span) noexcept -> Type*;

        template<typename Type>
        constexpr auto data_view(ice::Span<Type> span) noexcept -> ice::Data;

        template<typename Type>
        constexpr auto count(ice::Span<Type> span) noexcept -> ice::ucount;

        template<typename Type>
        constexpr auto size_bytes(ice::Span<Type> span) noexcept -> ice::usize;

        template<typename Type>
        constexpr auto alignment(ice::Span<Type> span) noexcept -> ice::ualign;

        template<typename Type>
        constexpr auto front(ice::Span<Type> span) noexcept -> Type&;

        template<typename Type>
        constexpr auto back(ice::Span<Type> span) noexcept -> Type&;

        template<typename Type>
        constexpr auto head(ice::Span<Type> span, ice::ucount count) noexcept -> ice::Span<Type>;

        template<typename Type>
        constexpr auto tail(ice::Span<Type> span, ice::ucount from_idx) noexcept -> ice::Span<Type>;

        template<typename Type>
        constexpr auto subspan(ice::Span<Type> span, ice::ucount from_idx, ice::ucount count = ice::ucount_max) noexcept -> ice::Span<Type>;

        template<typename Type>
        constexpr auto begin(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::Iterator;

        template<typename Type>
        constexpr auto end(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::Iterator;

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::ReverseIterator;

        template<typename Type>
        constexpr auto rend(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::ReverseIterator;

        template<typename Type>
        constexpr auto begin(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstIterator;

        template<typename Type>
        constexpr auto end(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstIterator;

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstReverseIterator;

        template<typename Type>
        constexpr auto rend(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstReverseIterator;


        template<typename Type, size_t Size>
        constexpr auto from_std(std::array<Type, Size> const& std_array) noexcept -> ice::Span<Type>;

        template<typename Type>
        constexpr auto memory(ice::Span<Type> span) noexcept -> ice::Memory;

    } // namespace span

    namespace data
    {

        template<typename T>
            requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        inline auto read_span(ice::Data source, ice::ucount count, ice::Span<T const>& out_value) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value._count = count;
            out_value._data = reinterpret_cast<T const*>(source.location);

            ice::usize const consumed_size = ice::size_of<T> *count;
            source.location = ice::span::data(out_value) + count;
            source.size = ice::usize::subtract(source.size, consumed_size);
            source.alignment = ice::align_of<T>;
            return source;
        }

    } // namespace data

    template<typename Type>
    constexpr Span<Type>::Span() noexcept
        : _count{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename Type>
    constexpr Span<Type>::Span(Type* location, ice::ucount count) noexcept
        : _count{ count }
        , _data{ location }
    {
    }

    template<typename Type>
    constexpr Span<Type>::Span(Type* from, Type* to) noexcept
        : _count{ static_cast<ice::ucount>(to - from) }
        , _data{ from }
    {
    }

    template<typename Type>
    template<ice::ucount Size>
    constexpr Span<Type>::Span(Type(&location)[Size]) noexcept
        : _count{ Size }
        , _data{ location }
    {
    }

    template<typename Type>
    constexpr auto Span<Type>::operator[](ice::ucount idx) const noexcept -> Type&
    {
        // TODO: ASSERT
        return _data[idx];
    }

    namespace span
    {

        template<typename Type>
        constexpr bool empty(ice::Span<Type> span) noexcept
        {
            return span._count == 0;
        }

        template<typename Type>
        constexpr bool any(ice::Span<Type> span) noexcept
        {
            return span._count != 0;
        }

        template<typename Type>
        constexpr auto data(ice::Span<Type> span) noexcept -> Type*
        {
            return span._data;
        }

        template<typename Type>
        constexpr auto data_view(ice::Span<Type> span) noexcept -> ice::Data
        {
            return Data{
                .location = ice::span::data(span),
                .size = ice::span::size_bytes(span),
                .alignment = ice::span::alignment(span)
            };
        }

        template<typename Type>
        constexpr auto count(ice::Span<Type> span) noexcept -> ice::ucount
        {
            return span._count;
        }

        template<typename Type>
        constexpr auto size_bytes(ice::Span<Type> span) noexcept -> ice::usize
        {
            return ice::size_of<Type> * ice::span::count(span);
        }

        template<typename Type>
        constexpr auto alignment(ice::Span<Type> span) noexcept -> ice::ualign
        {
            return ice::align_of<Type>;
        }

        template<typename Type>
        constexpr auto front(ice::Span<Type> span) noexcept -> Type&
        {
            return span._data[0];
        }

        template<typename Type>
        constexpr auto back(ice::Span<Type> span) noexcept -> Type&
        {
            return span._data[0];
        }

        template<typename Type>
        constexpr auto head(ice::Span<Type> span, ice::ucount count) noexcept -> ice::Span<Type>
        {
            ice::ucount const new_count = ice::min(count, span._count);
            return { span._data, new_count };
        }

        template<typename Type>
        constexpr auto tail(ice::Span<Type> span, ice::ucount from_idx) noexcept -> ice::Span<Type>
        {
            ice::ucount const from_start = ice::min(from_idx, span._count);
            return { span._data + from_start, span._count - from_start };
        }

        template<typename Type>
        constexpr auto subspan(ice::Span<Type> span, ice::ucount from_idx, ice::ucount count) noexcept -> ice::Span<Type>
        {
            ice::ucount const from_start = ice::min(from_idx, span._count);
            ice::ucount const new_count = ice::min(span._count - from_start, count);
            return { span._data + from_start, new_count };
        }

        template<typename Type>
        constexpr auto begin(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::Iterator
        {
            return span._data;
        }

        template<typename Type>
        constexpr auto end(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::Iterator
        {
            return span._data + span._count;
        }

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::ReverseIterator
        {
            return typename ice::Span<Type>::ReverseIterator{ span._data + span._count };
        }

        template<typename Type>
        constexpr auto rend(ice::Span<Type> span) noexcept -> typename ice::Span<Type>::ReverseIterator
        {
            return typename ice::Span<Type>::ReverseIterator{ span._data };
        }

        template<typename Type>
        constexpr auto begin(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstIterator
        {
            return span._data;
        }

        template<typename Type>
        constexpr auto end(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstIterator
        {
            return span._data + span._count;
        }

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstReverseIterator
        {
            return typename ice::Span<Type const>::ConstReverseIterator{ span._data + span._count };
        }

        template<typename Type>
        constexpr auto rend(ice::Span<Type const> span) noexcept -> typename ice::Span<Type const>::ConstReverseIterator
        {
            return typename ice::Span<Type const>::ConstReverseIterator{ span._data };
        }


        template<typename Type, size_t Size>
        constexpr auto from_std(std::array<Type, Size>& std_array) noexcept -> ice::Span<Type>
        {
            return ice::Span<Type>{ std_array.data(), ice::ucount(std_array.size()) };
        }

        template<typename Type, size_t Size>
        constexpr auto from_std_const(std::array<Type, Size> const& std_array) noexcept -> ice::Span<Type const>
        {
            return ice::Span<Type const>{ std_array.data(), ice::ucount(std_array.size()) };
        }

        // TODO: Move to another location or rename? Not sure this is properly named
        template<typename Type>
        constexpr auto memory(ice::Span<Type> span) noexcept -> ice::Memory
        {
            return ice::Memory{
                .location = ice::span::begin(span),
                .size = ice::span::size_bytes(span),
                .alignment = ice::span::alignment(span)
            };
        }

        template<typename Type>
        constexpr auto from_memory(ice::Memory const& mem, ice::meminfo meminfo, ice::usize offset) noexcept
        {
            void* const ptr = ice::ptr_add(mem.location, offset);
            ICE_ASSERT_CORE(ice::is_aligned(ptr, meminfo.alignment));
            ICE_ASSERT_CORE(ice::ptr_add(mem.location, mem.size) >= ice::ptr_add(ptr, meminfo.size));
            return ice::Span<Type>{ reinterpret_cast<Type*>(ptr), ice::ucount((meminfo.size / ice::size_of<Type>).value) };
        }

    } // namespace span

    using ice::span::count;
    using ice::span::data_view;

    using ice::span::begin;
    using ice::span::end;


    static_assert(ice::TrivialContainerLogicAllowed<ice::Span<ice::u32>>);

} // namespace ice
