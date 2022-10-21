
namespace ice
{

    template<ice::ucount Capacity, typename CharType>
    constexpr StaticString<Capacity, CharType>::StaticString() noexcept
        : _size{ 0 }
        , _data{ }
    {
    }

    template<ice::ucount Capacity, typename CharType>
    template<ice::ucount ArraySize>
    constexpr StaticString<Capacity, CharType>::StaticString(CharType const(&str_arr)[ArraySize]) noexcept
        : StaticString{ ice::BasicString<CharType>{ str_arr } }
    {
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr StaticString<Capacity, CharType>::StaticString(ice::BasicString<CharType> str) noexcept
        : _size{ ice::min(Capacity - 1, ice::string::size(str)) }
        , _data{ }
    {
        if (std::is_constant_evaluated())
        {
            for (ice::ucount idx = 0; idx < _size; ++idx)
            {
                _data[idx] = str[idx];
            }
        }
        else
        {
            ice::memcpy(ice::string::memory(*this), ice::string::data_view(str));
        }
        _data[_size] = ValueType{ 0 };
    }

    template<ice::ucount Capacity, typename CharType>
    template<ice::ucount OtherSize>
    constexpr StaticString<Capacity, CharType>::StaticString(StaticString<OtherSize, CharType> const& other) noexcept
        : _size{ ice::min(Capacity - 1, ice::string::size(other)) }
        , _data{ }
    {
        if (std::is_constant_evaluated())
        {
            for (ice::ucount idx = 0; idx < _size; ++idx)
            {
                _data[idx] = other[idx];
            }
        }
        else
        {
            ice::memcpy(ice::string::memory(*this), ice::string::data_view(other));
        }
        _data[_size] = ValueType{ 0 };
    }

    template<ice::ucount Capacity, typename CharType>
    template<ice::ucount OtherCapacity>
    constexpr auto StaticString<Capacity, CharType>::operator=(StaticString<OtherCapacity, CharType> const& other) noexcept -> StaticString<Capacity, CharType>&
    {
        if (this != &other)
        {
            ice::string::clear(*this);

            if (other._size > 0)
            {
                ice::memcpy(
                    ice::string::memory(*this),
                    ice::string::data_view(other)
                );
            }

            _size = ice::min(Capacity - 1, other._size);
            _data[_size] = CharType{ };
        }
        return *this;
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr auto StaticString<Capacity, CharType>::operator=(ice::BasicString<CharType> other) noexcept -> StaticString<Capacity, CharType>&
    {
        auto const* const other_str_begin = ice::string::begin(other);
        bool const part_of_this = other_str_begin >= ice::string::begin(*this)
            && other_str_begin < ice::string::end(*this);

        if (!part_of_this)
        {
            ice::string::clear(*this);

            ice::memcpy(
                ice::string::memory(*this),
                ice::string::data_view(other)
            );

            _size = ice::min(Capacity - 1, ice::string::size(other));
            _data[_size] = ValueType{ 0 };
        }
        return *this;
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr StaticString<Capacity, CharType>::operator ice::BasicString<CharType>() const noexcept
    {
        return { _data, _size };
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr bool operator==(ice::StaticString<Capacity, CharType> const& left, CharType const* right) noexcept
    {
        return ice::BasicString<CharType>{ left } == ice::BasicString<CharType>{ right };
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr bool operator==(ice::StaticString<Capacity, CharType> const& left, ice::BasicString<CharType> right) noexcept
    {
        return ice::BasicString<CharType>{ left } == right;
    }

    template<ice::ucount Capacity, typename CharType>
    constexpr bool operator==(ice::BasicString<CharType> left, ice::StaticString<Capacity, CharType> const& right) noexcept
    {
        return left == ice::BasicString<CharType>{ right };
    }

    namespace string
    {

        template<ice::ucount Capacity, typename CharType>
        constexpr void resize(ice::StaticString<Capacity, CharType>& str, ice::ucount new_size) noexcept
        {
            // #todo assert(new_size < Capacity)
            str._size = ice::min(Capacity - 1, new_size);
            str._data[str._size] = CharType{ 0 };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr void clear(ice::StaticString<Capacity, CharType>& str) noexcept
        {
            ice::string::resize(str, 0);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr void push_back(ice::StaticString<Capacity, CharType>& str, CharType character) noexcept
        {
            // #todo assert(new_size < Capacity)
            if (str._size + 1 < Capacity)
            {
                str._data[str._size] = character;
                str._size += 1;
                str._data[str._size] = CharType{ 0 };
            }
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr void push_back(ice::StaticString<Capacity, CharType>& str, CharType const* cstr) noexcept
        {
            return ice::string::push_back(str, ice::BasicString<CharType>{ cstr });
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr void push_back(ice::StaticString<Capacity, CharType>& str, ice::BasicString<CharType> other) noexcept
        {
            if (ice::string::empty(other) == false)
            {
                // #todo assert(new_size < Capacity)
                ice::ucount const new_size = ice::min(Capacity - 1, ice::string::size(str) + ice::string::size(other));
                ice::ucount const copy_size = new_size - ice::string::size(str);

                if (copy_size > 0)
                {
                    ice::memcpy(
                        ice::string::end(str),
                        ice::string::begin(other),
                        ice::size_of<CharType> * copy_size
                    );
                    str._size = new_size;
                    str._data[str._size] = 0;
                }
            }
        }

        template<ice::ucount CapacityDst, ice::ucount CapacitySrc, typename CharType>
        constexpr void push_back(ice::StaticString<CapacityDst, CharType>& str, ice::StaticString<CapacitySrc, CharType> other) noexcept
        {
            push_back(str, ice::String{ other });
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr void pop_back(ice::StaticString<Capacity, CharType>& str, ice::ucount count) noexcept
        {
            str._size = ice::min(0u, str._size - count);
            str._data[str._size] = CharType{ 0 };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto begin(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::Iterator
        {
            return str._data;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto end(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::Iterator
        {
            return str._data + str._size;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto rbegin(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ReverseIterator
        {
            return typename ice::HeapString<CharType>::ReverseIterator{ str._data + str._size };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto rend(ice::StaticString<Capacity, CharType>& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ReverseIterator
        {
            return typename ice::HeapString<CharType>::ReverseIterator{ str._data };
        }


        template<ice::ucount Capacity, typename CharType>
        constexpr auto size(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::ucount
        {
            return str._size;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto data(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ValueType const*
        {
            return str._data;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto capacity(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::ucount
        {
            return Capacity;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr bool empty(ice::StaticString<Capacity, CharType> const& str) noexcept
        {
            return str._size == 0;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto begin(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstIterator
        {
            return str._data;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto end(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstIterator
        {
            return str._data + str._size;
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto rbegin(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstReverseIterator
        {
            return typename ice::HeapString<CharType>::ConstReverseIterator{ str._data + str._size };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto rend(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ConstReverseIterator
        {
            return typename ice::HeapString<CharType>::ConstReverseIterator{ str._data };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto front(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ValueType
        {
            return str._data[0];
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto back(ice::StaticString<Capacity, CharType> const& str) noexcept -> typename ice::StaticString<Capacity, CharType>::ValueType
        {
            return str._data[str._size - 1];
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto substr(ice::StaticString<Capacity, CharType> const& str, ice::ucount pos, ice::ucount len) noexcept -> ice::BasicString<CharType>
        {
            return ice::string::substr(ice::BasicString<CharType>{ str }, pos, len);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto substr_clone(ice::StaticString<Capacity, CharType> const& str, ice::ucount pos, ice::ucount len) noexcept -> ice::StaticString<Capacity, CharType>
        {
            if (pos >= str._size)
            {
                return ice::StaticString<Capacity, CharType>{ };
            }

            ice::ucount pos_end = str._size;
            if (len != ice::String_NPos)
            {
                pos_end = ice::min(pos_end, pos + len);
            }

            return ice::StaticString<Capacity, CharType>{
                ice::BasicString<CharType>{ str._data + pos, str._data + pos_end }
            };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_first_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount
        {
            return ice::string::find_first_of(ice::BasicString<CharType>{ str }, character_value);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_first_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount
        {
            return ice::string::find_first_of(ice::BasicString<CharType>{ str }, character_values);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_last_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount
        {
            return ice::string::find_last_of(ice::BasicString<CharType>{ str }, character_value);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_last_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount
        {
            return ice::string::find_last_of(ice::BasicString<CharType>{ str }, character_values);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_first_not_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount
        {
            return ice::string::find_first_not_of(ice::BasicString<CharType>{ str }, character_value);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_first_not_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount
        {
            return ice::string::find_first_not_of(ice::BasicString<CharType>{ str }, character_values);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_last_not_of(ice::StaticString<Capacity, CharType> const& str, CharType character_value) noexcept -> ice::ucount
        {
            return ice::string::find_last_not_of(ice::BasicString<CharType>{ str }, character_value);
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto find_last_not_of(ice::StaticString<Capacity, CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount
        {
            return ice::string::find_last_not_of(ice::BasicString<CharType>{ str }, character_values);
        }


        template<ice::ucount Capacity, typename CharType>
        constexpr auto data_view(ice::StaticString<Capacity, CharType> const& str) noexcept -> ice::Data
        {
            return ice::Data{
                .location = str._data,
                .size = ice::size_of<CharType> * str._size,
                .alignment = ice::align_of<CharType>
            };
        }

        template<ice::ucount Capacity, typename CharType>
        constexpr auto memory(ice::StaticString<Capacity, CharType>& str) noexcept -> ice::Memory
        {
            return ice::Memory{
                .location = str._data,
                .size = ice::size_of<CharType> * Capacity,
                .alignment = ice::align_of<CharType>
            };
        }

    } // namespace string

} // namespace ice
