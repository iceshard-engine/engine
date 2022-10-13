#pragma once
#include <ice/base.hxx>
#include <ice/mem_data.hxx>

namespace ice
{

    //! \brief A view into an array of objects laid out in contiguous memory.
    template<typename Type>
    struct Span
    {
        using ValueType = Type;
        using Iterator = Type*;
        using ReverSeIterator = std::reverse_iterator<Type*>;
        using ConstIterator = Type const*;
        using ConstReverseIterator = std::reverse_iterator<Type const*>;

        ice::ucount _count;
        Type* _data;

        constexpr Span() noexcept;
        constexpr Span(ice::Span<Type>&& other) noexcept;
        constexpr Span(ice::Span<Type> const& other) noexcept;
        constexpr Span(Type* location, ice::ucount count) noexcept;
        constexpr Span(Type* from, Type* to) noexcept;

        template<ice::ucount Size>
        constexpr Span(Type(&location)[Size]) noexcept;

        constexpr auto operator=(ice::Span<Type>&& other) noexcept -> ice::Span<Type>&;
        constexpr auto operator=(ice::Span<Type> const& other) noexcept -> ice::Span<Type>&;

        constexpr auto operator[](ice::ucount idx) const noexcept -> Type&;
        constexpr operator ice::Span<Type const>() const noexcept { return { _data, _count }; }
    };

    namespace span
    {

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
        constexpr auto subspan(ice::Span<Type> span, ice::ucount from_idx, ice::ucount count) noexcept -> ice::Span<Type>;

        template<typename Type>
        constexpr auto begin(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::Iterator;

        template<typename Type>
        constexpr auto end(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::Iterator;

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::ReverseIterator;

        template<typename Type>
        constexpr auto rend(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::ReverseIterator;

    } // namespace span

    template<typename Type>
    constexpr Span<Type>::Span() noexcept
        : _count{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename Type>
    constexpr Span<Type>::Span(ice::Span<Type>&& other) noexcept
        : _count{ ice::exchange(other._count, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename Type>
    constexpr Span<Type>::Span(ice::Span<Type> const& other) noexcept
        : _count{ other._count }
        , _data{ other._data }
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
    constexpr auto Span<Type/*, Extent*/>::operator=(ice::Span<Type>&& other) noexcept -> ice::Span<Type>&
    {
        if (this != &other)
        {
            _count = ice::exchange(other._count, 0);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename Type>
    constexpr auto Span<Type>::operator=(ice::Span<Type> const& other) noexcept -> ice::Span<Type>&
    {
        if (this != &other)
        {
            _count = other._count;
            _data = other._data;
        }
        return *this;
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
        constexpr auto begin(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::Iterator
        {
            return span._data;
        }

        template<typename Type>
        constexpr auto end(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::Iterator
        {
            return span._data + span._count;
        }

        template<typename Type>
        constexpr auto rbegin(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::ReverseIterator
        {
            return { span._data + span._count };
        }

        template<typename Type>
        constexpr auto rend(ice::Span<Type>& span) noexcept -> typename ice::Span<Type>::ReverseIterator
        {
            return { span._data };
        }

    } // namespace span

    using ice::span::count;
    using ice::span::data_view;

    using ice::span::begin;
    using ice::span::end;

} // namespace ice
