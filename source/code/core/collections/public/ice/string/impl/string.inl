/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    constexpr auto operator""_str(char const* buffer, size_t size) noexcept -> ice::BasicString<char>
    {
        return ice::BasicString<char>{ buffer, ice::ucount(size) };
    }

    namespace string::detail
    {

        template<typename CharType>
        constexpr auto strptr_size(CharType const* str) noexcept -> ice::ucount
        {
            ice::ucount result = 0;
            if (str != nullptr)
            {
                CharType const* it = str;
                while (*it != CharType{ 0 })
                {
                    it += 1;
                }

                result = static_cast<ice::ucount>(it - str);
            }
            return result;
        }

    } // namespace string::detail

    template<typename CharType>
    constexpr BasicString<CharType>::BasicString(CharType const* str_ptr) noexcept
        : BasicString{ str_ptr, ice::string::detail::strptr_size(str_ptr) }
    {
    }

    template<typename CharType>
    constexpr BasicString<CharType>::BasicString(CharType const* str_beg, CharType const* str_end) noexcept
        : BasicString{ str_beg, static_cast<ice::ucount>(str_end - str_beg) }
    {
    }

    template<typename CharType>
    constexpr BasicString<CharType>::BasicString(CharType const* str_ptr, ice::ucount size) noexcept
        : _data{ str_ptr }
        , _size{ size }
    {
    }

    template<typename CharType>
    template<ice::ucount Size>
    constexpr BasicString<CharType>::BasicString(CharType const(&str_arr)[Size]) noexcept
        : _data{ str_arr }
        , _size{ Size }
    {
    }

    template<typename CharType>
    constexpr BasicString<CharType>::BasicString(std::basic_string_view<CharType> sv) noexcept
        : _data{ sv.data() }
        , _size{ ice::ucount(sv.size()) }
    {
    }

    template<typename CharType>
    constexpr auto BasicString<CharType>::operator[](ice::ucount index) const noexcept -> CharType const&
    {
        return _data[index];
    }

    template<typename CharType>
    constexpr bool BasicString<CharType>::operator==(BasicString other) const noexcept
    {
        if (_size == other._size)
        {
            if (_size == 0)
            {
                return true;
            }

            ice::ucount idx = 0;
            while (idx < _size && _data[idx] == other._data[idx])
            {
                idx += 1;
            }
            return idx == _size;
        }
        return false;
    }

    template<typename CharType>
    constexpr BasicString<CharType>::operator std::basic_string_view<CharType>() const noexcept
    {
        return { _data, _size };
    }

    namespace string
    {

        template<typename CharType>
        constexpr auto size(ice::BasicString<CharType> str) noexcept -> ice::ucount
        {
            return str._size;
        }

        template<typename CharType>
        constexpr auto capacity(ice::BasicString<CharType> str) noexcept -> ice::ucount
        {
            return ice::string::size(str);
        }

        template<typename CharType>
        constexpr bool empty(ice::BasicString<CharType> str) noexcept
        {
            return str._size == 0;
        }

        template<typename CharType>
        constexpr bool any(ice::BasicString<CharType> str) noexcept
        {
            return str._size != 0;
        }

        template<typename CharType>
        constexpr auto begin(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstIterator
        {
            return str._data;
        }

        template<typename CharType>
        constexpr auto end(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstIterator
        {
            return str._data + str._size;
        }

        template<typename CharType>
        constexpr auto rbegin(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstReverseIterator
        {
            return typename ice::BasicString<CharType>::ConstReverseIterator{ str._data + str._size };
        }

        template<typename CharType>
        constexpr auto rend(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ConstReverseIterator
        {
            return typename ice::BasicString<CharType>::ConstReverseIterator{ str._data };
        }

        template<typename CharType>
        constexpr auto front(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ValueType
        {
            return str[0];
        }

        template<typename CharType>
        constexpr auto back(ice::BasicString<CharType> str) noexcept -> typename ice::BasicString<CharType>::ValueType
        {
            return str[str._size - 1];
        }

        template<typename CharType>
        constexpr auto substr(ice::BasicString<CharType> str, ice::ucount pos, ice::ucount len) noexcept -> ice::BasicString<CharType>
        {
            if (pos >= str._size)
            {
                return { };
            }

            if (len == ice::String_NPos)
            {
                return { str._data + pos, str._size - pos };
            }
            else
            {
                return { str._data + pos, std::min(len, str._size - pos) };
            }
        }

        template<typename CharType>
        constexpr auto starts_with(ice::BasicString<CharType> str, ice::BasicString<CharType> prefix) noexcept
        {
            return ice::string::substr(str, 0, ice::string::size(prefix)) == prefix;
        }


        template<typename CharType>
        constexpr auto find_first_of(
            ice::BasicString<CharType> str,
            CharType character_value,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto const* it = ice::string::begin(str) + start_idx;
            auto const* const beg = it;
            auto const* const end = ice::string::end(str);

            while (it < end && *it != character_value)
            {
                it += 1;
            }

            return it >= end ? ice::String_NPos : start_idx + ice::ucount(it - beg);
        }

        template<typename CharType>
        constexpr auto find_first_of(
            ice::BasicString<CharType> str,
            ice::BasicString<CharType> character_values,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto const* it = ice::string::begin(str) + start_idx;
            auto const* const beg = it;
            auto const* const it_end = ice::string::end(str);

            while (it < it_end && ice::string::find_first_of(character_values, *it) == ice::String_NPos)
            {
                it += 1;
            }

            return it >= it_end ? ice::String_NPos : start_idx + ice::ucount(it - beg);
        }

        template<typename CharType>
        constexpr auto find_last_of(
            ice::BasicString<CharType> str,
            CharType character_value,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != it_end && *it != character_value)
            {
                it += 1;
            }

            return it == it_end ? ice::String_NPos : ice::ucount(it_end - it) - 1;
        }

        template<typename CharType>
        constexpr auto find_last_of(
            ice::BasicString<CharType> str,
            ice::BasicString<CharType> character_values,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::String_NPos)
            {
                it += 1;
            }

            return it == it_end ? ice::String_NPos : ice::ucount(it_end - it) - 1;
        }

        template<typename CharType>
        constexpr auto find_first_not_of(
            ice::BasicString<CharType> str,
            CharType character_value,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto const* it = ice::string::begin(str) + start_idx;
            auto const* const beg = it;
            auto const* const end = ice::string::end(str);

            while (it < end && *it == character_value)
            {
                it += 1;
            }

            return it >= end ? ice::String_NPos : start_idx + ice::ucount(it - beg);
        }

        template<typename CharType>
        constexpr auto find_first_not_of(
            ice::BasicString<CharType> str,
            ice::BasicString<CharType> character_values,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto const* it = ice::string::begin(str) + start_idx;
            auto const* const beg = it;
            auto const* const it_end = ice::string::end(str);

            while (it < it_end && ice::string::find_first_of(character_values, *it) != ice::String_NPos)
            {
                it += 1;
            }

            return it >= it_end ? ice::String_NPos : start_idx + ice::ucount(it - beg);
        }

        template<typename CharType>
        constexpr auto find_last_not_of(
            ice::BasicString<CharType> str,
            CharType character_value,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto it = ice::string::rbegin(str);
            auto const end = ice::string::rend(str);

            while (it != end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != end && *it == character_value)
            {
                it += 1;
            }

            return it == end ? ice::String_NPos : ice::ucount(end - it) - 1;
        }

        template<typename CharType>
        constexpr auto find_last_not_of(
            ice::BasicString<CharType> str,
            ice::BasicString<CharType> character_values,
            ice::ucount start_idx
        ) noexcept -> ice::ucount
        {
            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != it_end && ice::string::find_first_of(character_values, *it) != ice::String_NPos)
            {
                it += 1;
            }

            return it == it_end ? ice::String_NPos : ice::ucount(it_end - it) - 1;
        }


        template<typename T, typename CharType> requires ice::concepts::RODataObject<T>
        constexpr auto from_data(T ro_data) noexcept -> ice::BasicString<CharType>
        {
            return ice::string::from_data(ro_data, 0_B, static_cast<ice::ucount>(ro_data.size.value));
        }

        template<typename T, typename CharType> requires ice::concepts::RODataObject<T>
        constexpr auto from_data(T ro_data, ice::usize offset, ice::ucount size) noexcept -> ice::String
        {
            return ice::String{
                reinterpret_cast<char const*>(ro_data.location) + offset.value,
                ice::min(static_cast<ice::ucount>(ro_data.size.value), size)
            };
        }

        template<typename CharType>
        constexpr auto data_view(ice::BasicString<CharType> str) noexcept -> typename ice::Data
        {
            return ice::Data{
                .location = str._data,
                .size = ice::size_of<CharType> * str._size,
                .alignment = ice::align_of<CharType>
            };
        }

    } // namespace string

} // namespace ice
