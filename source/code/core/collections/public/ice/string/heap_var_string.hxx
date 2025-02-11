/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/string/var_string.hxx>

namespace ice::string
{

    template<typename CharType>
    inline void clear(ice::HeapVarString<CharType>& str) noexcept;

    template<typename CharType>
    inline auto begin(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::Iterator;

    template<typename CharType>
    inline auto end(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::Iterator;

    template<typename CharType>
    inline auto rbegin(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::ReverseIterator;

    template<typename CharType>
    inline auto rend(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::ReverseIterator;

    template<typename CharType>
    inline auto deserialize(ice::HeapVarString<CharType>& str, ice::Data data) noexcept -> ice::Data;

} // namespace ice::string

namespace ice
{

    // using ice::string::size;
    // using ice::string::begin;
    // using ice::string::end;

} // namespace ice

#include "impl/heap_var_string.inl"
