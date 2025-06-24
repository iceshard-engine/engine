/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>

namespace ice::string
{

    template<typename CharType>
    constexpr auto size(ice::BasicString<CharType> str) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto capacity(ice::BasicString<CharType> str) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr bool empty(ice::BasicString<CharType> str) noexcept;

    template<typename CharType>
    constexpr bool any(ice::BasicString<CharType> str) noexcept;

    template<typename CharType>
    constexpr auto begin(ice::BasicString<CharType> a) noexcept -> typename ice::BasicString<CharType>::ConstIterator;

    template<typename CharType>
    constexpr auto end(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstIterator;

    template<typename CharType>
    constexpr auto rbegin(ice::BasicString<CharType> a) noexcept -> typename ice::BasicString<CharType>::ConstReverseIterator;

    template<typename CharType>
    constexpr auto rend(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstReverseIterator;

    template<typename CharType>
    constexpr auto front(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ValueType;

    template<typename CharType>
    constexpr auto back(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ValueType;

    template<typename CharType>
    constexpr auto substr(ice::BasicString<CharType> str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::BasicString<CharType>;

    template<typename CharType>
    constexpr auto starts_with(ice::BasicString<CharType> str, ice::BasicString<CharType> prefix) noexcept;

    template<typename CharType>
    constexpr auto find_first_of(
        ice::BasicString<CharType> str,
        CharType character_value,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_first_of(
        ice::BasicString<CharType> str,
        ice::BasicString<CharType> character_values,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_of(
        ice::BasicString<CharType> str,
        CharType character_value,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_of(
        ice::BasicString<CharType> str,
        ice::BasicString<CharType> character_values,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_first_not_of(
        ice::BasicString<CharType> str,
        CharType character_value,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_first_not_of(
        ice::BasicString<CharType> str,
        ice::BasicString<CharType> character_values,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_not_of(
        ice::BasicString<CharType> str,
        CharType character_value,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_not_of(
        ice::BasicString<CharType> str,
        ice::BasicString<CharType> character_values,
        ice::ucount start_idx = 0
    ) noexcept -> ice::ucount;


    template<typename T, typename CharType = char> requires ice::concepts::RODataObject<T>
    constexpr auto from_data(T ro_data) noexcept -> ice::BasicString<CharType>;

    template<typename T, typename CharType = char> requires ice::concepts::RODataObject<T>
    constexpr auto from_data(T ro_data, ice::usize offset, ice::ucount size) noexcept -> ice::String;

    template<typename CharType>
    constexpr auto data_view(ice::BasicString<CharType> str) noexcept -> typename ice::Data;

    template<typename CharType>
    constexpr auto meminfo(ice::BasicString<CharType> str) noexcept -> ice::meminfo;

} // namespace ice::string

namespace ice
{

    using ice::string::size;
    using ice::string::begin;
    using ice::string::end;

} // namespace ice

#include "impl/string.inl"
