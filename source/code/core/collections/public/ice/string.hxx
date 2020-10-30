#pragma once
#include <ice/string_types.hxx>

namespace ice::string
{

    template<typename CharType>
    constexpr auto size(ice::BasicString<CharType> str) noexcept -> uint32_t;

    template<typename CharType>
    constexpr auto length(ice::BasicString<CharType> str) noexcept -> uint32_t;

    template<typename CharType>
    constexpr auto data(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type const*;

    template<typename CharType>
    constexpr auto capacity(ice::BasicString<CharType> str) noexcept -> uint32_t;

    template<typename CharType>
    constexpr bool empty(ice::BasicString<CharType> str) noexcept;

    template<typename CharType>
    constexpr auto begin(ice::BasicString<CharType> a) noexcept -> typename ice::BasicString<CharType>::const_iterator;

    template<typename CharType>
    constexpr auto end(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_iterator;

    template<typename CharType>
    constexpr auto rbegin(ice::BasicString<CharType> a) noexcept -> typename ice::BasicString<CharType>::const_reverse_iterator;

    template<typename CharType>
    constexpr auto rend(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_reverse_iterator;

    template<typename CharType>
    constexpr auto front(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type;

    template<typename CharType>
    constexpr auto back(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type;

    template<typename CharType>
    constexpr auto substr(ice::BasicString<CharType> str, uint32_t pos, uint32_t len = ice::string_npos) noexcept -> ice::BasicString<CharType>;

    template<typename CharType>
    constexpr auto find_first_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> uint32_t;

    template<typename CharType>
    constexpr auto find_first_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> uint32_t;

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

    template<typename CharType>
    constexpr bool equals(ice::BasicString<CharType> left, ice::BasicString<CharType> right) noexcept;


    //////////////////////////////////////////////////////////////////////////


    template<typename CharType>
    constexpr auto size(ice::BasicString<CharType> str) noexcept -> uint32_t
    {
        return static_cast<uint32_t>(str.size());
    }

    template<typename CharType>
    constexpr auto length(ice::BasicString<CharType> str) noexcept -> uint32_t
    {
        return static_cast<uint32_t>(str.length());
    }

    template<typename CharType>
    constexpr auto data(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type const*
    {
        return str.data();
    }

    template<typename CharType>
    constexpr auto capacity(ice::BasicString<CharType> str) noexcept -> uint32_t
    {
        return ice::string::size(str);
    }

    template<typename CharType>
    constexpr bool empty(ice::BasicString<CharType> str) noexcept
    {
        return str.empty();
    }

    template<typename CharType>
    constexpr auto begin(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_iterator
    {
        return str.cbegin();
    }

    template<typename CharType>
    constexpr auto end(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_iterator
    {
        return str.cend();
    }

    template<typename CharType>
    constexpr auto rbegin(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_reverse_iterator
    {
        return str.crbegin();
    }

    template<typename CharType>
    constexpr auto rend(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::const_reverse_iterator
    {
        return str.crend();
    }

    template<typename CharType>
    constexpr auto front(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type
    {
        return str.front();
    }

    template<typename CharType>
    constexpr auto back(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::value_type
    {
        return str.back();
    }

    template<typename CharType>
    constexpr auto substr(ice::BasicString<CharType> str, uint32_t pos, uint32_t len) noexcept -> ice::BasicString<CharType>
    {
        return str.substr(pos, len == ice::string_npos ? std::string_view::npos : len);
    }

    template<typename CharType>
    constexpr auto find_first_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> uint32_t
    {
        auto const result = str.find_first_of(character_value);
        return result == std::string::npos ? ice::string_npos : static_cast<uint32_t>(result);
    }

    template<typename CharType>
    constexpr auto find_first_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
    {
        auto const result = str.find_first_of(character_values);
        return result == std::string::npos ? ice::string_npos : static_cast<uint32_t>(result);
    }

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, CharType character_value) noexcept -> uint32_t
    {
        auto const result = str.find_last_of(character_value);
        return result == std::string::npos ? ice::string_npos : static_cast<uint32_t>(result);
    }

    template<typename CharType>
    constexpr auto find_last_of(ice::BasicString<CharType> str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
    {
        auto const result = str.find_last_of(character_values);
        return result == std::string::npos ? ice::string_npos : static_cast<uint32_t>(result);
    }

    template<typename CharType>
    constexpr bool equals(ice::BasicString<CharType> left, ice::BasicString<CharType> right) noexcept
    {
        return left == right;
    }

} // namespace ice::string
