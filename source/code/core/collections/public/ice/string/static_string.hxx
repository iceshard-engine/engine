#pragma once
#include <ice/string_types.hxx>
#include <ice/string/string.hxx>

namespace ice::string
{

    template<ice::ucount Capacity, typename CharType>
    constexpr void set_capacity(ice::StaticString<Capacity, CharType>& str, ice::ucount new_capacity) noexcept = delete;

    template<ice::ucount Capacity, typename CharType>
    constexpr void reserve(ice::StaticString<Capacity, CharType>& str, ice::ucount min_capacity) noexcept = delete;

    template<ice::ucount Capacity, typename CharType>
    constexpr void grow(ice::StaticString<Capacity, CharType>& str, ice::ucount min_capacity) noexcept = delete;

    template<ice::ucount Capacity, typename CharType>
    constexpr void resize(ice::StaticString<Capacity, CharType>& str, ice::ucount new_size) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr void shrink(ice::StaticString<Capacity, CharType>& str) noexcept = delete;

    template<ice::ucount Capacity, typename CharType>
    constexpr void clear(ice::StaticString<Capacity, CharType>& str) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr void push_back(ice::StaticString<Capacity, CharType>& str, CharType character) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr void push_back(ice::StaticString<Capacity, CharType>& str, CharType const* cstr) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr void push_back(ice::StaticString<Capacity, CharType>& str, ice::BasicString<CharType> cstr) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr void pop_back(ice::StaticString<Capacity, CharType>& str, ice::ucount count) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto begin(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::Iterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto end(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::Iterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto rbegin(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ReverseIterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto rend(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ReverseIterator;


    template<ice::ucount Capacity, typename CharType>
    constexpr auto size(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto capacity(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr bool empty(ice::StaticString<Capacity, CharType> const& str) noexcept;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto begin(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstIterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto end(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstIterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto rbegin(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstReverseIterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto rend(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstReverseIterator;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto front(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ValueType;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto back(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ValueType;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto substr(ice::StaticString<Capacity, CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::BasicString<CharType>;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto substr_clone(ice::StaticString<Capacity, CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::StaticString<Capacity, CharType>;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_first_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_first_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_last_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_last_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_first_not_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_first_not_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_last_not_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto find_last_not_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;


    template<ice::ucount Capacity, typename CharType>
    constexpr auto data_view(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::Data;

    template<ice::ucount Capacity, typename CharType>
    constexpr auto memory(ice::StaticString<Capacity, CharType>& str) noexcept -> ice::Memory;

} // namespace ice::string

namespace ice
{

    using ice::string::size;
    using ice::string::begin;
    using ice::string::end;

} // namespace ice

#include "impl/static_string.inl"
