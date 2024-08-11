/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/string/string.hxx>

namespace ice::string
{

    template<typename CharType>
    inline void set_capacity(ice::HeapString<CharType>& str, ice::ucount new_capacity) noexcept;

    template<typename CharType>
    inline void reserve(ice::HeapString<CharType>& str, ice::ucount min_capacity) noexcept;

    template<typename CharType>
    inline void grow(ice::HeapString<CharType>& str, ice::ucount min_capacity = 0) noexcept;

    template<typename CharType>
    inline void resize(ice::HeapString<CharType>& str, ice::ucount new_size) noexcept;

    template<typename CharType>
    inline void shrink(ice::HeapString<CharType>& str) noexcept;

    template<typename CharType>
    inline void clear(ice::HeapString<CharType>& str) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapString<CharType>& str, CharType character) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapString<CharType>& str, CharType const* cstr) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapString<CharType>& str, ice::HeapString<CharType> const& other) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapString<CharType>& str, ice::BasicString<CharType> cstr) noexcept;

    template<typename CharType>
    inline void pop_back(ice::HeapString<CharType>& str, ice::ucount count = 1) noexcept;

    template<typename CharType>
    inline auto begin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::Iterator;

    template<typename CharType>
    inline auto end(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::Iterator;

    template<typename CharType>
    inline auto rbegin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::ReverseIterator;

    template<typename CharType>
    inline auto rend(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::ReverseIterator;


    template<typename CharType>
    inline auto size(ice::HeapString<CharType> const& str) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto capacity(ice::HeapString<CharType> const& str) noexcept -> ice::ucount;

    template<typename CharType>
    inline bool empty(ice::HeapString<CharType> const& str) noexcept;

    template<typename CharType>
    inline auto begin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstIterator;

    template<typename CharType>
    inline auto end(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstIterator;

    template<typename CharType>
    inline auto rbegin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstReverseIterator;

    template<typename CharType>
    inline auto rend(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstReverseIterator;

    template<typename CharType>
    inline auto front(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ValueType;

    template<typename CharType>
    inline auto back(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ValueType;

    template<typename CharType>
    inline auto substr(ice::HeapString<CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::BasicString<CharType>;

    template<typename CharType>
    inline auto substr_clone(ice::HeapString<CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::HeapString<CharType>;

    template<typename CharType>
    inline auto find_first_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_first_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_last_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_last_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;


    template<typename CharType>
    inline auto data_view(ice::HeapString<CharType> const& str) noexcept -> ice::Data;

    template<typename CharType>
    inline auto memory(ice::HeapString<CharType>& str) noexcept -> ice::Memory;

    template<typename CharType>
    inline auto extract_memory(ice::HeapString<CharType>& str) noexcept -> ice::Memory;

} // namespace ice::string

namespace ice
{

    using ice::string::size;
    using ice::string::begin;
    using ice::string::end;

} // namespace ice

#include "impl/heap_string.inl"
