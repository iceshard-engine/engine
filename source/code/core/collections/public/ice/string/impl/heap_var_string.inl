/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    namespace string::detail
    {

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
    inline HeapVarString<CharType>::HeapVarString(ice::HeapVarString<CharType>&& other) noexcept
        : _allocator{ other._allocator }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename CharType>
    inline HeapVarString<CharType>::HeapVarString(ice::HeapVarString<CharType> const& other) noexcept
        : HeapVarString{ other._allocator, ice::String{ other } }
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
    inline bool operator==(ice::HeapVarString<CharType> const& left, CharType const* right) noexcept
    {
        return ice::BasicString<CharType>{ left } == ice::BasicString<CharType>{ right };
    }

    template<typename CharType>
    inline bool operator==(ice::HeapVarString<CharType> const& left, ice::BasicString<CharType> right) noexcept
    {
        return ice::BasicString<CharType>{ left } == right;
    }

    template<typename CharType>
    inline bool operator==(ice::BasicString<CharType> left, ice::HeapVarString<CharType> const& right) noexcept
    {
        return left == ice::BasicString<CharType>{ right };
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

    template<typename CharType>
    inline HeapVarString<CharType>::operator ice::VarStringBase<CharType>() const noexcept
    {
        return _data;
    }

    namespace string
    {

        template<typename CharType>
        inline void clear(ice::HeapVarString<CharType>& str) noexcept
        {
            str._allocator->deallocate(ice::exchange(str._data, nullptr));
        }

        template<typename CharType>
        inline auto begin(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::Iterator
        {
            return ice::string::detail::data_varstring(str._data);
        }

        template<typename CharType>
        inline auto end(ice::HeapVarString<CharType>& str) noexcept -> typename ice::HeapVarString<CharType>::Iterator
        {
            ice::ucount bytes;
            ice::ucount const size = ice::string::detail::read_varstring_size(str._data, bytes);
            return str._data + bytes + size;
        }

        template<typename CharType>
        auto deserialize(ice::HeapVarString<CharType>& str, ice::Data data) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(data.size >= 2_B); // 1 byte for size + 1 for a single character
            ice::string::clear(str); // Clear the current contents

            ice::ucount bytes;
            ice::ucount const size = ice::string::detail::read_varstring_size(
                reinterpret_cast<char const*>(data.location), bytes
            );
            if (size > 0)
            {
                char* const new_str = ice::string::detail::allocate_varstring_exact(*str._allocator, size, bytes);
                if (new_str != nullptr)
                {
                    ice::memcpy(new_str + bytes, ice::ptr_add(data.location, ice::usize{ bytes }), size);
                    new_str[bytes + size] = '\0';
                }
                str._data = new_str; // Assign the new allocated data
            }

            return ice::ptr_add(data, { bytes + size });
        }

    } // namespace string

} // namespace ice
