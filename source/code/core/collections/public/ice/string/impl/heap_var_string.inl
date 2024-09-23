/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    namespace string::detail
    {

        inline auto write_varstring_size(void* data, ice::ucount size) noexcept -> ice::ucount
        {
            ice::ucount bytes = 0;
            ice::u8* var_byte = reinterpret_cast<ice::u8*>(data);
            while(size > 0x7f)
            {
                var_byte[bytes] = (size & 0x7f) | 0x80;
                size >>= 7;
                bytes += 1;
            }
            var_byte[bytes] = size & 0x7f;
            return bytes + 1;
        }

        inline auto allocate_varstring_exact(ice::Allocator& alloc, ice::ucount size, ice::ucount& out_size_bytes) noexcept -> char*
        {
            if (size == 0)
            {
                return nullptr;
            }

            // Allocate enough for: bytes + size + '\0'
            ice::ucount const final_size = ice::string::detail::calc_varstring_required_size(size) + 1;
            ice::Memory const result = alloc.allocate(ice::usize{ final_size });
            out_size_bytes = write_varstring_size(result.location, size);
            return reinterpret_cast<char*>(result.location);
        }

        inline auto create_varstring(ice::Allocator& alloc, ice::String str) noexcept -> char*
        {
            ice::ucount const str_size = ice::string::size(str);

            ice::ucount bytes = 0;
            char* data = ice::string::detail::allocate_varstring_exact(alloc, str_size, bytes);
            if (data != nullptr)
            {
                ice::memcpy(data + bytes, ice::string::begin(str), str_size);
                data[bytes + str_size] = '\0';
            }
            return data;
        }

    } // namespace string::detail

    template<typename CharType>
    inline HeapVarString<CharType>::HeapVarString(ice::Allocator& alloc) noexcept
        : _allocator{ ice::addressof(alloc) }
        , _data{ nullptr }
    {
    }

    template<typename CharType>
    inline HeapVarString<CharType>::HeapVarString(ice::Allocator& alloc, ice::BasicString<CharType> string) noexcept
        : _allocator{ ice::addressof(alloc) }
        , _data{ ice::string::detail::create_varstring(alloc, string) }
    {
    }

    template<typename CharType>
    inline HeapVarString<CharType>::~HeapVarString() noexcept
    {
        if (_data != nullptr)
        {
            _allocator->deallocate(_data);
        }
    }

    template<typename CharType>
    inline auto HeapVarString<CharType>::operator=(ice::BasicString<CharType> str) noexcept -> HeapVarString&
    {
        if (_data != nullptr)
        {
            _allocator->deallocate(_data);
        }

        _data = ice::string::detail::create_varstring(*_allocator, str);
        return *this;
    }

    template<typename CharType>
    inline HeapVarString<CharType>::operator ice::BasicString<CharType>() const noexcept
    {
        ice::ucount bytes = 0;
        ice::ucount const size = ice::string::detail::read_varstring_size(_data, bytes);
        if (size > 0)
        {
            return { _data + bytes, size };
        }
        else
        {
            return {};
        }
    }


} // namespace ice
