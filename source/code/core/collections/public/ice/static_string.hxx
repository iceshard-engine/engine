/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT
#pragma once
#include <ice/string/string_concepts.hxx>
#include <ice/string/resizable_operations.hxx>
#include <ice/string.hxx>

namespace ice
{

    template<ice::u32 Capacity = 12, typename CharT = char> requires ice::concepts::SupportedCharType<CharT>
    struct StaticString : ice::string::MutableOperations
    {
        using CharType = CharT;
        using ValueType = CharType;
        using Iterator = CharType*;
        using ReverseIterator = std::reverse_iterator<CharType*>;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;
        using SizeType = ice::ncount;
        using StringType = ice::BasicString<CharType>;

        SizeType::base_type _size;
        ValueType _data[Capacity];

        constexpr StaticString() noexcept;
        template<u32 Size>
        constexpr StaticString(CharType const(&str_array)[Size]) noexcept;
        constexpr StaticString(BasicString<CharType> string) noexcept;

        constexpr auto operator=(StaticString const& other) noexcept -> StaticString&;
        constexpr auto operator=(BasicString<CharType> str) noexcept -> StaticString&;

        constexpr operator ice::BasicString<CharType>() const noexcept;

        // ReadOnly+ Operations
        template<typename Self>
        constexpr auto data(this Self& self) noexcept { return self._data; }
        constexpr auto size() const noexcept -> SizeType { return SizeType{ _size, sizeof(CharType) }; }

        // Mutable operations
        constexpr void resize(ice::ncount new_size) noexcept;
        constexpr auto capacity() const noexcept -> SizeType { return SizeType{ Capacity, sizeof(CharType) }; }

        // Data info
        constexpr auto data_view() const noexcept -> ice::Data;
    };

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr StaticString<Capacity, CharT>::StaticString() noexcept
        : _size{ 0 }
        , _data{ }
    {
    }

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    template<ice::u32 Size>
    constexpr StaticString<Capacity, CharT>::StaticString(CharType const(&str_array)[Size]) noexcept
        : StaticString{ ice::BasicString<CharType>{ str_array, Size } }
    {
    }

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr StaticString<Capacity, CharT>::StaticString(ice::BasicString<CharT> str) noexcept
        : _size{ ice::min(Capacity - 1, str.size().u32()) }
        , _data{ }
    {
        if (std::is_constant_evaluated())
        {
            for (ice::u32 idx = 0; idx < _size; ++idx)
            {
                _data[idx] = str[idx];
            }
        }
        else if (str.not_empty())
        {
            ice::memcpy(this->memory_view(), str.data_view());
        }
        _data[_size] = ValueType{ 0 };
    }

    //template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    //template<ice::u32 OtherSize>
    //constexpr StaticString<Capacity, CharType>::StaticString(StaticString<OtherSize, CharType> const& other) noexcept
    //    : _size{ ice::min(Capacity - 1, ice::string::size(other)) }
    //    , _data{ }
    //{
    //    if (std::is_constant_evaluated())
    //    {
    //        for (ice::ucount idx = 0; idx < _size; ++idx)
    //        {
    //            _data[idx] = other[idx];
    //        }
    //    }
    //    else
    //    {
    //        ice::memcpy(ice::string::memory(*this), ice::string::data_view(other));
    //    }
    //    _data[_size] = ValueType{ 0 };
    //}

    //template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    //template<ice::ucount OtherCapacity>
    //constexpr auto StaticString<Capacity, CharType>::operator=(StaticString<OtherCapacity, CharType> const& other) noexcept -> StaticString<Capacity, CharType>&
    //{
    //    if (this != &other)
    //    {
    //        ice::string::clear(*this);

    //        if (other._size > 0)
    //        {
    //            ice::memcpy(
    //                ice::string::memory(*this),
    //                ice::string::data_view(other)
    //            );
    //        }

    //        _size = ice::min(Capacity - 1, other._size);
    //        _data[_size] = CharType{ };
    //    }
    //    return *this;
    //}

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr auto StaticString<Capacity, CharT>::operator=(ice::BasicString<CharType> other) noexcept -> StaticString&
    {
        CharType const* const other_str_begin = other.begin();
        bool const part_of_this = other_str_begin >= this->begin()
            && other_str_begin < this->end();

        if (!part_of_this)
        {
            this->clear();
            ice::memcpy(this->memory_view(), other.data_view());
            this->resize(ice::min<ncount>(Capacity - 1, other.size()));
        }
        return *this;
    }

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr StaticString<Capacity, CharT>::operator ice::BasicString<typename StaticString<Capacity, CharT>::CharType>() const noexcept
    {
        return ice::BasicString<CharType>{ _data, _size };
    }

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr void StaticString<Capacity, CharT>::resize(ice::ncount new_size) noexcept
    {
        _size = ice::min(Capacity - 1, new_size.u32());
        _data[_size] = CharType{ 0 };
    }

    //template<ice::u32 Capacity, typename CharType>
    //constexpr bool operator==(ice::StaticString<Capacity, CharType> const& left, CharType const* right) noexcept
    //{
    //    return ice::BasicString<CharType>{ left } == ice::BasicString<CharType>{ right };
    //}

    //template<ice::u32 Capacity, typename CharType>
    //constexpr bool operator==(ice::StaticString<Capacity, CharType> const& left, ice::BasicString<CharType> right) noexcept
    //{
    //    return ice::BasicString<CharType>{ left } == right;
    //}

    //template<ice::u32 Capacity, typename CharType>
    //constexpr bool operator==(ice::BasicString<CharType> left, ice::StaticString<Capacity, CharType> const& right) noexcept
    //{
    //    return left == ice::BasicString<CharType>{ right };
    //}

    template<ice::u32 Capacity, typename CharT> requires ice::concepts::SupportedCharType<CharT>
    constexpr auto StaticString<Capacity, CharT>::data_view() const noexcept -> ice::Data
    {
        return Data{
            .location = _data,
            .size = size().bytes(),
            .alignment = ice::align_of<CharType>
        };
    }

} // namespace ice
