#pragma once
#include <ice/string_types.hxx>

namespace ice::string
{

    template<typename CharType>
    constexpr auto size(ice::BasicString<CharType> str) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto capacity(ice::BasicString<CharType> str) noexcept -> ice::ucount;

    template<typename CharType>
        //requires std::is_same_v<CharType, ice::utf8> // TODO: utf-16, utf-32?
    constexpr auto utf8_codepoints(ice::BasicString<CharType> str) noexcept -> ice::ucount;


    template<typename CharType>
    constexpr bool empty(ice::BasicString<CharType> str) noexcept;

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
    constexpr auto find_first_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_first_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;


    template<typename CharType>
    constexpr auto data_view(ice::BasicString<CharType> str) noexcept -> typename ice::Data;

} // namespace ice::string

namespace ice
{

    using ice::string::size;
    using ice::string::begin;
    using ice::string::end;

} // namespace ice

#include "impl/string.inl"
