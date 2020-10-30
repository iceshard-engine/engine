#pragma once
#include <ice/string_types.hxx>
#include <ice/string.hxx>

namespace ice
{

    namespace string
    {

        template<uint32_t Size, typename CharType>
        inline void set_capacity(ice::StackString<Size, CharType>& str, uint32_t new_capacity) noexcept = delete;

        template<uint32_t Size, typename CharType>
        inline void reserve(ice::StackString<Size, CharType>& str, uint32_t min_capacity) noexcept = delete;

        template<uint32_t Size, typename CharType>
        inline void grow(ice::StackString<Size, CharType>& str, uint32_t min_capacity = 0) noexcept = delete;

        template<uint32_t Size, typename CharType>
        inline void resize(ice::StackString<Size, CharType>& str, uint32_t new_size) noexcept;

        template<uint32_t Size, typename CharType>
        inline void shrink(ice::StackString<Size, CharType>& str) noexcept = delete;

        template<uint32_t Size, typename CharType>
        inline void clear(ice::StackString<Size, CharType>& str) noexcept;

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, CharType character) noexcept;

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, CharType const* cstr) noexcept;

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, ice::StackString<Size, CharType> const& other) noexcept;

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, ice::BasicString<CharType> cstr) noexcept;

        template<uint32_t Size, typename CharType>
        inline auto begin(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::iterator;

        template<uint32_t Size, typename CharType>
        inline auto end(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::iterator;

        template<uint32_t Size, typename CharType>
        inline auto rbegin(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::reverse_iterator;

        template<uint32_t Size, typename CharType>
        inline auto rend(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::reverse_iterator;


        template<uint32_t Size, typename CharType>
        inline auto size(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline auto length(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline auto data(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type const*;

        template<uint32_t Size, typename CharType>
        inline auto capacity(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline bool empty(ice::StackString<Size, CharType> const& str) noexcept;

        template<uint32_t Size, typename CharType>
        inline auto begin(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_iterator;

        template<uint32_t Size, typename CharType>
        inline auto end(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_iterator;

        template<uint32_t Size, typename CharType>
        inline auto rbegin(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_reverse_iterator;

        template<uint32_t Size, typename CharType>
        inline auto rend(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_reverse_iterator;

        template<uint32_t Size, typename CharType>
        inline auto front(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type;

        template<uint32_t Size, typename CharType>
        inline auto back(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type;

        template<uint32_t Size, typename CharType>
        inline auto substr(ice::StackString<Size, CharType> const& str, uint32_t pos, uint32_t len = ice::string_npos) noexcept -> ice::BasicString<CharType>;

        template<uint32_t Size, typename CharType>
        inline auto substr_clone(ice::StackString<Size, CharType> const& str, uint32_t pos, uint32_t len = ice::string_npos) noexcept -> ice::StackString<Size, CharType>;

        template<uint32_t Size, typename CharType>
        inline auto find_first_of(ice::StackString<Size, CharType> const& str, CharType character_value) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline auto find_first_of(ice::StackString<Size, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline auto find_last_of(ice::StackString<Size, CharType> const& str, CharType character_value) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline auto find_last_of(ice::StackString<Size, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

        template<uint32_t Size, typename CharType>
        inline bool equals(ice::StackString<Size, CharType> const& left, ice::StackString<Size, CharType> const& right) noexcept;

    } // namespace string

    template<uint32_t Size, typename CharType>
    inline StackString<Size, CharType>::StackString(ice::BasicString<CharType> value) noexcept
    {
        *this = value;
    }

    template<uint32_t Size, typename CharType>
    template<uint32_t OtherSize>
    inline StackString<Size, CharType>::StackString(StackString<OtherSize, CharType> const& other) noexcept
    {
        uint32_t const required_capacity = other._size + 1;
        ice::string::set_capacity(*this, required_capacity);

        _size = other._size;
        ice::memcpy(_data, other._data, sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
    }

    template<uint32_t Size, typename CharType>
    template<uint32_t OtherSize>
    inline auto StackString<Size, CharType>::operator=(StackString<OtherSize, CharType> const& other) noexcept -> StackString<Size, CharType>&
    {
        _size = ice::min(Size - 1, ice::string::size(other));
        ice::memcpy(_data, ice::string::data(other), sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
        return *this;
    }

    template<uint32_t Size, typename CharType>
    inline auto StackString<Size, CharType>::operator=(ice::BasicString<CharType> other) noexcept -> StackString<Size, CharType>&
    {
        _size = ice::min(Size -1, ice::string::size(other));
        ice::memcpy(_data, ice::string::data(other), sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
        return *this;
    }

    template<uint32_t Size, typename CharType>
    inline StackString<Size, CharType>::operator ice::BasicString<CharType>() const noexcept
    {
        return { _data, _size };
    }

    template<uint32_t Size, typename CharType>
    inline auto operator==(StackString<Size, CharType> const& left, CharType const* right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), ice::BasicString<CharType>{ right });
    }

    template<uint32_t Size, typename CharType>
    inline auto operator==(StackString<Size, CharType> const& left, ice::BasicString<CharType> right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), right);
    }

    template<uint32_t Size, typename CharType>
    inline auto operator==(BasicString<CharType> left, ice::StackString<Size, CharType> const& right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), right.operator ice::String());
    }

    namespace string
    {

        template<uint32_t Size, typename CharType>
        inline void resize(ice::StackString<Size, CharType>& str, uint32_t new_size) noexcept
        {
            // #todo assert
            str._size = ice::min(Size - 1, new_size);
            str._data[str._size] = CharType{ 0 };
        }

        template<uint32_t Size, typename CharType>
        inline void clear(ice::StackString<Size, CharType>& str) noexcept
        {
            ice::string::resize(str, 0);
        }

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, CharType character) noexcept
        {
            // #todo assert
            if (str._size + 1 < Size)
            {
                str._data[str._size] = character;
                str._size += 1;
                str._data[str._size] = CharType{ 0 };
            }
        }

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, CharType const* cstr) noexcept
        {
            return ice::string::push_back(str, ice::BasicString<CharType>{ cstr });
        }

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, ice::StackString<Size, CharType> const& other) noexcept
        {
            return ice::string::push_back(str, other.operator ice::BasicString<CharType>());
        }

        template<uint32_t Size, typename CharType>
        inline void push_back(ice::StackString<Size, CharType>& str, ice::BasicString<CharType> other) noexcept
        {
            if (ice::string::empty(other) == false)
            {
                // #todo assert
                uint32_t const new_size = ice::min(Size - 1, ice::string::size(str) + ice::string::size(other));
                uint32_t const copy_size = new_size - ice::string::size(str);

                if (copy_size > 0)
                {
                    ice::memcpy(ice::string::end(str), ice::string::data(other), copy_size);
                    str._size = new_size;
                    str._data[str._size] = 0;
                }
            }
        }

        template<uint32_t Size, typename CharType>
        inline auto begin(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::iterator
        {
            return str._data;
        }

        template<uint32_t Size, typename CharType>
        inline auto end(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::iterator
        {
            return str._data + str._size;
        }

        template<uint32_t Size, typename CharType>
        inline auto rbegin(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data + str._size };
        }

        template<uint32_t Size, typename CharType>
        inline auto rend(ice::StackString<Size, CharType>& str) noexcept -> typename ice::StackString<Size, CharType>::reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data };
        }


        template<uint32_t Size, typename CharType>
        inline auto size(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t
        {
            return str._size;
        }

        template<uint32_t Size, typename CharType>
        inline auto length(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t
        {
            auto const it_beg = ice::string::begin(str);
            auto const it_end = ice::string::end(str);
            auto it = it_beg;
            while (it != it_end && *it != '\0')
            {
                it += 1;
            }
            return static_cast<uint32_t>(it - it_beg);
        }

        template<uint32_t Size, typename CharType>
        inline auto data(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type const*
        {
            return str._data;
        }

        template<uint32_t Size, typename CharType>
        inline auto capacity(ice::StackString<Size, CharType> const& str) noexcept -> uint32_t
        {
            return Size;
        }

        template<uint32_t Size, typename CharType>
        inline bool empty(ice::StackString<Size, CharType> const& str) noexcept
        {
            return str._size == 0;
        }

        template<uint32_t Size, typename CharType>
        inline auto begin(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_iterator
        {
            return str._data;
        }

        template<uint32_t Size, typename CharType>
        inline auto end(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_iterator
        {
            return str._data + str._size;
        }

        template<uint32_t Size, typename CharType>
        inline auto rbegin(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data + str._size };
        }

        template<uint32_t Size, typename CharType>
        inline auto rend(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::const_reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data };
        }

        template<uint32_t Size, typename CharType>
        inline auto front(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type
        {
            return str._data[0];
        }

        template<uint32_t Size, typename CharType>
        inline auto back(ice::StackString<Size, CharType> const& str) noexcept -> typename ice::StackString<Size, CharType>::value_type
        {
            return str._data[str._size - 1];
        }

        template<uint32_t Size, typename CharType>
        inline auto substr(ice::StackString<Size, CharType> const& str, uint32_t pos, uint32_t len) noexcept -> ice::BasicString<CharType>
        {
            return ice::string::substr(str.operator ice::String(), pos, len);
        }

        template<uint32_t Size, typename CharType>
        inline auto substr_clone(ice::StackString<Size, CharType> const& str, uint32_t pos, uint32_t len) noexcept -> ice::StackString<Size, CharType>
        {
            if (pos >= str._size)
            {
                return ice::StackString<Size, CharType>{ *str._allocator };
            }

            uint32_t pos_end = str._size;
            if (len != ice::string_npos)
            {
                pos_end = ice::min(pos_end, pos + len);
            }

            return ice::StackString<Size, CharType>{
                *str._allocator,
                ice::String{ str._data + pos, str._data + pos_end }
            };
        }

        template<uint32_t Size, typename CharType>
        inline auto find_first_of(ice::StackString<Size, CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto const it = ice::string::begin(str);
            auto const it_end = ice::string::end(str);

            while(it != it_end && *it != character_value)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : result;
        }

        template<uint32_t Size, typename CharType>
        inline auto find_first_of(ice::StackString<Size, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto const it = ice::string::begin(str);
            auto const it_end = ice::string::end(str);

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::string_npos)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : result;
        }

        template<uint32_t Size, typename CharType>
        inline auto find_last_of(ice::StackString<Size, CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto const it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && *it != character_value)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : result;
        }

        template<uint32_t Size, typename CharType>
        inline auto find_last_of(ice::StackString<Size, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto const it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::string_npos)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : result;
        }

        template<uint32_t Size, typename CharType>
        inline bool equals(ice::StackString<Size, CharType> const& left, ice::StackString<Size, CharType> const& right) noexcept
        {
            return ice::string::equals(ice::BasicString<CharType>{ left }, ice::BasicString<CharType>{ right });
        }

    } // namespace string

} // namespace ice
