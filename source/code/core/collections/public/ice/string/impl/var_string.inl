/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    namespace string::detail
    {

        inline auto calc_varstring_required_size(ice::ucount size) noexcept -> ice::ucount
        {
            ice::ucount bytes = 0;
            while(size > 0x7f)
            {
                size >>= 7;
                bytes += 1;
            }
            return (bytes + 1) + size;
        }

        inline auto read_varstring_size(char const* data, ice::ucount& out_bytes) noexcept -> ice::ucount
        {
            ice::ucount result = 0;
            if (data != nullptr)
            {
                ice::u8 const* var_byte = reinterpret_cast<ice::u8 const*>(data);
                out_bytes = 1;
                while(*var_byte & 0x80)
                {
                    result += *var_byte;
                    result <<= 7;
                    var_byte += 1;
                    out_bytes += 1;
                }
                result += *var_byte;
            }
            return result;
        }

        inline auto read_varstring_size(char const* data) noexcept -> ice::ucount
        {
            ice::ucount bytes; // Unused
            return read_varstring_size(data, bytes);
        }

        template<typename CharType>
        inline auto data_varstring(CharType const* data) noexcept -> CharType const*
        {
            ice::ucount bytes = 0;
            if (data == nullptr || read_varstring_size(data, bytes) == 0)
            {
                return nullptr;
            }
            return data + bytes;
        }

        inline auto write_varstring_size(void* data, ice::ucount size) noexcept -> ice::ucount
        {
            ice::ucount bytes = 0;
            ice::u8* var_byte = reinterpret_cast<ice::u8*>(data);
            while (size > 0x7f)
            {
                var_byte[bytes] = (size & 0x7f) | 0x80;
                size >>= 7;
                bytes += 1;
            }
            var_byte[bytes] = size & 0x7f;
            return bytes + 1;
        }

    } // namespace detail

    template<typename CharType>
    inline VarStringBase<CharType>::VarStringBase() noexcept
        : _data{ nullptr }
    { }

    template<typename CharType>
    inline VarStringBase<CharType>::VarStringBase(CharType const* str_ptr) noexcept
        : _data{ str_ptr }
    {
    }

    template<typename CharType>
    inline auto VarStringBase<CharType>::operator[](ice::ucount idx) noexcept -> CharType&
    {
        ice::ucount bytes = 0;
        ice::ucount const size = read_varstring_size(_data, bytes);
        ICE_ASSERT_CORE(size > idx);
        return (_data + bytes + idx);
    }

    template<typename CharType>
    inline auto VarStringBase<CharType>::operator[](ice::ucount idx) const noexcept -> CharType const&
    {
        ice::ucount bytes = 0;
        ice::ucount const size = read_varstring_size(_data, bytes);
        ICE_ASSERT_CORE(size > idx);
        return (_data + bytes + idx);
    }

    template<typename CharType>
    inline VarStringBase<CharType>::operator ice::BasicString<CharType>() const noexcept
    {
        ice::u32 bytes = 0;
        ice::ucount const size = ice::string::detail::read_varstring_size(_data, bytes);
        return { _data + bytes, size };
    }

    namespace string
    {

        inline auto size(ice::string::VarStringType auto const& str) noexcept -> ice::ucount
        {
            return ice::string::detail::read_varstring_size(str._data);
        }

        inline auto capacity(ice::string::VarStringType auto const& str) noexcept -> ice::ucount
        {
            return ice::string::size(str);
        }

        inline bool empty(ice::string::VarStringType auto const& str) noexcept
        {
            return str._data == nullptr || str._data[0] == '\0';
        }

        template<ice::string::VarStringType StringType>
        inline auto begin(StringType const& str) noexcept -> typename StringType::ConstIterator
        {
            return ice::string::detail::data_varstring(str._data);
        }

        template<ice::string::VarStringType StringType>
        inline auto end(StringType const& str) noexcept -> typename StringType::ConstIterator
        {
            ice::ucount bytes = 0;
            ice::ucount const size = ice::string::detail::read_varstring_size(str._data, bytes);
            return str._data + bytes + size;
        }

        inline auto data_view(ice::string::VarStringType auto const& str) noexcept -> ice::Data
        {
            ice::ucount bytes = 0;
            ice::ucount const size = ice::string::detail::read_varstring_size(str._data, bytes);

            return {
                .location = str._data,
                .size = { size + bytes },
                .alignment = ice::ualign::b_1
            };
        }

        auto serialize(ice::string::VarStringType auto const& str, ice::Memory target) noexcept -> ice::Memory
        {
            ice::ucount const size = ice::string::size(str);
            ICE_ASSERT_CORE(
                ice::string::detail::calc_varstring_required_size(size) <= target.size.value
            );

            ice::ucount const sizebytes = ice::string::detail::write_varstring_size(target.location, size);
            target.location = ice::ptr_add(target.location, ice::usize{ sizebytes });
            target.size.value -= sizebytes;

            ice::memcpy(target.location, str._data + sizebytes, size);
            target.location = ice::ptr_add(target.location, ice::usize{ size });
            target.size.value -= size;
            target.alignment = ice::ualign::b_1;
            return target;
        }

    } // namespace string

    namespace data
    {

        template<typename CharType>
        inline auto read_varstring(ice::Data data, ice::VarStringBase<CharType>& out_str) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(data.size >= 2_B); // 1 byte for size + 1 for a single character

            char const* const rawstr = reinterpret_cast<char const*>(data.location);

            ice::ucount bytes;
            ice::ucount const size = ice::string::detail::read_varstring_size(rawstr, bytes);
            if (size > 0)
            {
                out_str._data = rawstr;
            }

            return ice::ptr_add(data, { bytes + size });
        }

    } // namespace data

} // namespace ice
