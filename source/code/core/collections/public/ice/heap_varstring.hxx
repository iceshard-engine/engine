/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/varstring.hxx>
#include <ice/string/resizable_operations.hxx>

namespace ice
{

    template<typename CharT = char>
    struct HeapVarString : ice::string::ResizableOperations
    {
        static_assert(sizeof(CharT) == 1, "Wider characters are not supported yet!");

        using TypeTag = VarStringTag;
        using CharType = CharT;
        using ValueType = CharType;
        using Iterator = CharType*;
        using ReverseIterator = std::reverse_iterator<CharType*>;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;
        using SizeType = ice::ncount;
        using StringType = ice::BasicString<CharType>;

        ice::Allocator* _allocator;
        ValueType* _data;

        constexpr explicit HeapVarString(ice::Allocator& alloc) noexcept;
        constexpr HeapVarString(ice::Allocator& allocator, ice::BasicString<CharType> string) noexcept;
        constexpr ~HeapVarString() noexcept;

        constexpr HeapVarString(HeapVarString&& other) noexcept;
        constexpr HeapVarString(HeapVarString const& other) noexcept;

        constexpr auto data() const noexcept -> ValueType*;
        constexpr auto size() const noexcept -> SizeType;

        constexpr void clear() noexcept;
        constexpr auto deserialize(ice::Data data) noexcept -> ice::Data;

        constexpr auto data_view() const noexcept -> ice::Data;

        constexpr operator ice::BasicString<CharType>() const noexcept;
        constexpr operator ice::VarStringBase<CharType>() const noexcept;
    };

    namespace varstring
    {

        inline auto allocate_exact(ice::Allocator& alloc, ice::ncount size, ice::usize& out_bytes) noexcept -> char*
        {
            if (size == 0_B)
            {
                return nullptr;
            }

            // Allocate enough for: bytes + size + '\0'
            ice::usize const final_size = calc_required_size(size) + 1_B;
            ice::Memory const result = alloc.allocate(final_size);
            out_bytes = write_size(result.location, size);
            return reinterpret_cast<char*>(result.location);
        }

        inline auto create(ice::Allocator& alloc, ice::String str) noexcept -> char*
        {
            ice::ncount const str_size = str.size();

            ice::usize bytes = 0_B;
            char* const data = allocate_exact(alloc, str_size, bytes);
            if (data != nullptr)
            {
                ice::memcpy(data + bytes.value, str.begin(), str_size.bytes());
                data[bytes.value + str_size.native()] = '\0';
            }
            return data;
        }

    } // namespace string::detail

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::HeapVarString(ice::Allocator& alloc) noexcept
        : _allocator{ ice::addressof(alloc) }
        , _data{ nullptr }
    {
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::HeapVarString(ice::Allocator& alloc, ice::BasicString<CharT> string) noexcept
        : _allocator{ ice::addressof(alloc) }
        , _data{ ice::varstring::create(alloc, string) }
    {
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::HeapVarString(ice::HeapVarString<CharT>&& other) noexcept
        : _allocator{ other._allocator }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::HeapVarString(ice::HeapVarString<CharT> const& other) noexcept
        : HeapVarString{ other._allocator, ice::String{ other } }
    {
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::~HeapVarString() noexcept
    {
        if (_data != nullptr)
        {
            _allocator->deallocate(_data);
        }
    }

    template<typename CharT>
    inline constexpr auto HeapVarString<CharT>::data() const noexcept -> ValueType*
    {
        return ice::varstring::read_data(_data);
    }

    template<typename CharT>
    inline constexpr auto HeapVarString<CharT>::size() const noexcept -> SizeType
    {
        return ice::varstring::read_size(_data);
    }

    template<typename CharT>
    inline constexpr void HeapVarString<CharT>::clear() noexcept
    {
        _allocator->deallocate(std::exchange(_data, nullptr));
    }

    template<typename CharT>
    inline constexpr auto HeapVarString<CharT>::deserialize(ice::Data data) noexcept -> ice::Data
    {
        ICE_ASSERT_CORE(data.size >= 2_B); // 1 byte for size + 1 for a single character
        this->clear(); // Clear the current contents

        ice::usize bytes;
        ice::ncount const size = ice::varstring::read_size(reinterpret_cast<char const*>(data.location), bytes);
        if (size > 0)
        {
            char* const new_str = ice::varstring::allocate_exact(*_allocator, size, bytes);
            if (new_str != nullptr)
            {
                ice::memcpy(new_str + bytes.value, ice::ptr_add(data.location, bytes), size.bytes());
                new_str[bytes.value + size.native()] = '\0';
            }
            _data = new_str; // Assign the new allocated data
        }

        return ice::ptr_add(data, bytes + size.bytes());
    }

    template<typename CharT>
    inline constexpr auto HeapVarString<CharT>::data_view() const noexcept -> ice::Data
    {
        ice::usize bytes = 0_B;
        ice::ncount const size = ice::varstring::read_size(_data, bytes);

        return {
            .location = _data,
            .size = { size.bytes() + bytes},
            .alignment = ice::ualign::b_1
        };
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::operator ice::BasicString<typename HeapVarString<CharT>::CharType>() const noexcept
    {
        ice::usize bytes = 0_B;
        ice::ncount const size = ice::varstring::read_size(_data, bytes);
        if (size > 0)
        {
            return { _data + bytes.value, size };
        }
        else
        {
            return {};
        }
    }

    template<typename CharT>
    inline constexpr HeapVarString<CharT>::operator ice::VarStringBase<typename HeapVarString<CharT>::CharType>() const noexcept
    {
        return _data;
    }

} // namespace ice
