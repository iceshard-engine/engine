/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/string/string_concepts.hxx>
#include <ice/string/readonly_operations.hxx>

namespace ice
{

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    struct BasicString : ice::string::ReadOnlyOperations
    {
        using CharType = CharT;
        using ValueType = CharType const;
        using ConstIterator = ValueType*;
        using ConstReverseIterator = std::reverse_iterator<ValueType*>;
        using Iterator = ConstIterator;
        using ReverseIterator = ConstReverseIterator;
        using SizeType = ice::ncount;
        using StringType = ice::BasicString<CharType>;

        ValueType* _data = nullptr;
        SizeType::base_type _count = 0;

        constexpr BasicString() noexcept = default;

        constexpr BasicString(BasicString&& other) noexcept = default;
        constexpr BasicString(BasicString const& other) noexcept = default;

        constexpr auto operator=(BasicString&& other) noexcept -> BasicString& = default;
        constexpr auto operator=(BasicString const& other) noexcept -> BasicString& = default;

        constexpr BasicString(ValueType* ptr_array_nt) noexcept
            : _data{ ptr_array_nt }
            , _count{ ice::string::detail::strptr_size(ptr_array_nt) }
        { }

        constexpr BasicString(ValueType* ptr_array, ice::ncount count) noexcept
            : _data{ ptr_array }
            , _count{ count.native() }
        { }

        constexpr BasicString(ValueType* ptr_array_begin, ValueType* ptr_array_end) noexcept
            : _data{ ptr_array_begin }
            , _count{ static_cast<ice::u64>(ptr_array_end - ptr_array_begin) }
        { }

        template<ice::u64 Size>
        constexpr BasicString(ValueType(&char_array)[Size]) noexcept
            : _data{ char_array }
            , _count{ Size }
        { }

        constexpr BasicString(std::basic_string_view<CharType> string_view) noexcept
            : _data{ string_view.data() }
            , _count{ string_view.size() }
        { }

        constexpr auto data() const noexcept -> ValueType* { return _data; }
        constexpr auto size() const noexcept -> SizeType { return SizeType{ _count, sizeof(CharType) }; }

        constexpr auto data_view() const noexcept -> ice::Data;

        constexpr operator std::basic_string_view<CharType>() const noexcept { return { _data, _count }; }
    };

    static_assert(ice::concepts::StringType<ice::BasicString<char>>);


    constexpr auto operator""_str(char const* buffer, size_t size) noexcept -> ice::BasicString<char>
    {
        return ice::BasicString<char>{ buffer, size };
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr auto BasicString<CharT>::data_view() const noexcept -> ice::Data
    {
        return Data{
            .location = _data,
            .size = size().bytes(),
            .alignment = ice::align_of<CharType>
        };
    }

    using String = ice::BasicString<char>;
    using WString = ice::BasicString<wchar_t>;

    template<typename CharType, typename T> requires ice::concepts::RODataObject<T>
    constexpr auto string_from_data(T ro_data, ice::nindex offset, ice::ncount size) noexcept -> ice::BasicString<CharType>
    {
        return ice::String{
            reinterpret_cast<CharType const*>(ro_data.location) + offset,
            ice::min<ice::ncount::base_type>(ro_data.size.value, size)
        };
    }

    template<typename CharType, typename T> requires ice::concepts::RODataObject<T>
    constexpr auto string_from_data(T ro_data) noexcept -> ice::BasicString<CharType>
    {
        return ice::string_from_data<CharType>(ro_data, 0, ro_data.size.value);
    }

    constexpr auto hash(ice::String value) noexcept -> ice::u64
    {
        return ice::hash(std::string_view{ value });
    }

    constexpr auto hash32(ice::String value) noexcept -> ice::u32
    {
        return ice::hash32(std::string_view{ value });
    }

    constexpr auto stringid(ice::String value) noexcept -> ice::StringID
    {
        return ice::stringid(value.data(), value.size().u64());
    }

} // namespace ice
