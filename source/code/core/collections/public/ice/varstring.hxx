/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>
#include <ice/string/string_concepts.hxx>
#include <ice/string/readonly_operations.hxx>

namespace ice
{

    using VarStringTag = struct _tagVarString;

    template<typename CharT = char>
    struct VarStringBase : ice::string::ReadOnlyOperations
    {
        static_assert(sizeof(CharT) == 1, "Wider characters are not supported yet!");

        using CharType = CharT;
        using ValueType = CharType const;
        using ConstIterator = ValueType*;
        using ConstReverseIterator = std::reverse_iterator<ValueType*>;
        using Iterator = ConstIterator;
        using ReverseIterator = ConstReverseIterator;
        using SizeType = ice::ncount;
        using StringType = ice::BasicString<CharType>;
        using TypeTag = VarStringTag;

        ValueType* _data;

        inline VarStringBase() noexcept;
        inline ~VarStringBase() noexcept = default;

        inline explicit VarStringBase(CharType const* data) noexcept;

        constexpr auto data() const noexcept -> ValueType*;
        constexpr auto size() const noexcept -> SizeType;

        inline operator ice::BasicString<CharType>() const noexcept;
    };

    using VarString = VarStringBase<char>;
    //using VarWString = VarStringBase<ice::wchar>;

    namespace varstring
    {

        inline auto calc_required_size(ice::ncount size) noexcept -> ice::usize
        {
            ice::ncount::base_type temp = size.native();
            ice::usize bytes = 0_B;
            while (size > 0x7f)
            {
                temp >>= 7;
                bytes += 1_B;
            }
            return size.bytes() + bytes + 1_B;
        }

        inline auto read_size(char const* data, ice::usize& out_bytes) noexcept -> ice::ncount
        {
            ice::ncount::base_type result = 0;
            if (data != nullptr)
            {
                ice::u8 const* var_byte = reinterpret_cast<ice::u8 const*>(data);
                out_bytes = 1_B;
                while (*var_byte & 0x80)
                {
                    result += *var_byte;
                    result <<= 7;
                    var_byte += 1;
                    out_bytes += 1_B;
                }
                result += *var_byte;
            }
            return ice::ncount{ result, sizeof(char) };
        }

        inline auto read_size(char const* data) noexcept -> ice::ncount
        {
            ice::usize bytes;
            return read_size(data, bytes);
        }

        template<typename CharType>
        inline auto read_data(CharType* data) noexcept -> CharType*
        {
            ice::usize bytes = 0_B;
            if (data == nullptr || read_size(data, bytes) == 0)
            {
                return nullptr;
            }
            return data + bytes.value;
        }

        inline auto write_size(void* data, ice::ncount size) noexcept -> ice::ncount
        {
            ice::ncount::base_type temp = size;
            ice::nindex byte = 0;
            ice::u8* const var_byte = reinterpret_cast<ice::u8*>(data);
            while (size > 0x7f)
            {
                var_byte[byte] = (size & 0x7f) | 0x80;
                temp >>= 7;
                byte += 1;
            }
            var_byte[byte] = size & 0x7f;
            return byte + 1;
        }

    } // namespace varstring

    template<typename CharT>
    inline VarStringBase<CharT>::VarStringBase() noexcept
        : _data{ nullptr }
    {
    }

    template<typename CharT>
    inline VarStringBase<CharT>::VarStringBase(CharT const* str_ptr) noexcept
        : _data{ str_ptr }
    {
    }

    template<typename CharT>
    inline constexpr auto VarStringBase<CharT>::data() const noexcept -> ValueType*
    {
        return ice::varstring::read_data(_data);
    }

    template<typename CharT>
    inline constexpr auto VarStringBase<CharT>::size() const noexcept -> SizeType
    {
        return ice::varstring::read_size(_data);
    }

    template<typename CharT>
    inline VarStringBase<CharT>::operator ice::BasicString<typename VarStringBase<CharT>::CharType>() const noexcept
    {
        ice::usize bytes = 0;
        ice::ncount const size = ice::varstring::read_size(_data, bytes);
        return { _data + bytes.value, size };
    }

} // namespace ice
