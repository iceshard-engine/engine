#pragma once
#include <ice/string_types.hxx>
#include <ice/string.hxx>

namespace ice
{

    namespace string
    {

        template<typename CharType>
        inline void set_capacity(ice::HeapString<CharType>& str, uint32_t new_capacity) noexcept;

        template<typename CharType>
        inline void reserve(ice::HeapString<CharType>& str, uint32_t min_capacity) noexcept;

        template<typename CharType>
        inline void grow(ice::HeapString<CharType>& str, uint32_t min_capacity = 0) noexcept;

        template<typename CharType>
        inline void resize(ice::HeapString<CharType>& str, uint32_t new_size) noexcept;

        template<typename CharType>
        inline void shrink(ice::HeapString<CharType>& str) noexcept;

        template<typename CharType>
        inline void clear(ice::HeapString<CharType>& str) noexcept;

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, CharType character) noexcept;

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, CharType const* cstr) noexcept;

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, ice::HeapString<CharType> const& other) noexcept;

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, ice::BasicString<CharType> cstr) noexcept;

        template<typename CharType>
        inline void pop_back(ice::HeapString<CharType>& str, uint32_t count = 1) noexcept;

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::iterator;

        template<typename CharType>
        inline auto end(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::iterator;

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::reverse_iterator;

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::reverse_iterator;


        template<typename CharType>
        inline auto size(ice::HeapString<CharType> const& str) noexcept -> uint32_t;

        template<typename CharType>
        inline auto length(ice::HeapString<CharType> const& str) noexcept -> uint32_t;

        template<typename CharType>
        inline auto data(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type const*;

        template<typename CharType>
        inline auto capacity(ice::HeapString<CharType> const& str) noexcept -> uint32_t;

        template<typename CharType>
        inline bool empty(ice::HeapString<CharType> const& str) noexcept;

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_iterator;

        template<typename CharType>
        inline auto end(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_iterator;

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_reverse_iterator;

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_reverse_iterator;

        template<typename CharType>
        inline auto front(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type;

        template<typename CharType>
        inline auto back(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type;

        template<typename CharType>
        inline auto substr(ice::HeapString<CharType> const& str, uint32_t pos, uint32_t len = ice::string_npos) noexcept -> ice::BasicString<CharType>;

        template<typename CharType>
        inline auto substr_clone(ice::HeapString<CharType> const& str, uint32_t pos, uint32_t len = ice::string_npos) noexcept -> ice::HeapString<CharType>;

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t;

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t;

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t;

        template<typename CharType>
        inline bool equals(ice::HeapString<CharType> const& left, ice::HeapString<CharType> const& right) noexcept;

    } // namespace string

    template<typename CharType>
    inline HeapString<CharType>::HeapString(ice::Allocator& allocator) noexcept
        : _allocator{ &allocator }
    {
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(ice::Allocator& allocator, ice::BasicString<CharType> value) noexcept
        : _allocator{ &allocator }
    {
        *this = value;
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(ice::HeapString<CharType>&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ other._size }
        , _capacity{ other._capacity }
        , _data{ other._data }
    {
        other._data = nullptr;
        other._capacity = 0;
        other._size = 0;
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(HeapString<CharType> const& other) noexcept
        : _allocator{ other._allocator }
    {
        uint32_t const required_capacity = other._size + 1;
        ice::string::set_capacity(*this, required_capacity);

        _size = other._size;
        ice::memcpy(_data, other._data, sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
    }

    template<typename CharType>
    inline HeapString<CharType>::~HeapString() noexcept
    {
        _allocator->deallocate({ .result = _data, .size = ice::size_of<CharType> *_capacity, .alignment = ice::align_of<CharType> });
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(HeapString<CharType>&& other) noexcept -> HeapString<CharType>&
    {
        if (this != &other)
        {
            ice::swap(_allocator, other._allocator);
            ice::swap(_size, other._size);
            ice::swap(_capacity, other._capacity);
            ice::swap(_data, other._data);
        }
        return *this;
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(HeapString<CharType> const& other) noexcept -> HeapString<CharType>&
    {
        uint32_t const required_capacity = other._size + 1;
        ice::string::set_capacity(*this, required_capacity);

        _size = other._size;
        ice::memcpy(_data, ice::string::data(other), sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
        return *this;
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(ice::BasicString<CharType> str) noexcept -> HeapString<CharType>&
    {
        uint32_t const string_size = ice::string::size(str);
        uint32_t const required_capacity = string_size + 1;
        ice::string::set_capacity(*this, required_capacity);

        _size = string_size;
        ice::memcpy(_data, ice::string::data(str), sizeof(value_type) * _size);
        _data[_size] = value_type{ 0 };
        return *this;
    }

    template<typename CharType>
    inline auto operator==(HeapString<CharType> const& left, CharType const* right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), ice::BasicString<CharType>{ right });
    }

    template<typename CharType>
    inline auto operator==(HeapString<CharType> const& left, ice::BasicString<CharType> right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), right);
    }

    template<typename CharType>
    inline auto operator==(BasicString<CharType> left, ice::HeapString<CharType> const& right) noexcept
    {
        return ice::string::equals(left.operator ice::String(), right.operator ice::String());
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator[](uint32_t index) noexcept -> CharType&
    {
        return _data[index];
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator[](uint32_t index) const noexcept -> CharType const&
    {
        return _data[index];
    }

    template<typename CharType>
    inline HeapString<CharType>::operator ice::BasicString<CharType>() const noexcept
    {
        return { _data, _size };
    }

    namespace string
    {

        template<typename CharType>
        inline void set_capacity(ice::HeapString<CharType>& str, uint32_t new_capacity) noexcept
        {
            using value_type = typename HeapString<CharType>::value_type;

            if (new_capacity == str._capacity)
            {
                return;
            }

            value_type* new_buffer = nullptr;
            if (new_capacity > 0)
            {
                ice::AllocResult new_data = str._allocator->allocate(ice::size_of<value_type> * new_capacity);

                if (str._size > 0)
                {
                    int32_t const copy_size = ice::min(str._size + 1, new_capacity);
                    ice::memcpy(new_data.result, str._data, sizeof(value_type) * copy_size);
                }

                new_buffer = reinterpret_cast<value_type*>(new_data.result);
            }

            str._allocator->deallocate({ .result = str._data, .size = ice::size_of<CharType> * str._capacity, .alignment = ice::align_of<CharType> });
            str._data = new_buffer;
            str._size = ice::min(new_capacity, str._size);
            str._capacity = new_capacity;
        }

        template<typename CharType>
        inline void reserve(ice::HeapString<CharType>& str, uint32_t min_capacity) noexcept
        {
            if (min_capacity > str._capacity)
            {
                ice::string::set_capacity(str, min_capacity);
            }
        }

        template<typename CharType>
        inline void grow(ice::HeapString<CharType>& str, uint32_t min_capacity) noexcept
        {
            uint32_t new_capacity = str._capacity * 2 + 8;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }

            ice::string::set_capacity(str, new_capacity);
        }

        template<typename CharType>
        inline void resize(ice::HeapString<CharType>& str, uint32_t new_size) noexcept
        {
            if ((new_size + 1) > str._capacity)
            {
                ice::string::grow(str, new_size + 1);
            }
            str._size = new_size;
            str._data[str._size] = CharType{ 0 };
        }

        template<typename CharType>
        inline void shrink(ice::HeapString<CharType>& str) noexcept
        {
            ice::string::set_capacity(str, ice::string::size(str) + 1);
        }

        template<typename CharType>
        inline void clear(ice::HeapString<CharType>& str) noexcept
        {
            ice::string::resize(str, 0);
        }

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, CharType character) noexcept
        {
            if (str._size + 1 > str._capacity)
            {
                ice::string::grow(str);
            }

            str._data[str._size] = character;
            str._size += 1;
            str._data[str._size] = CharType{ 0 };
        }

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, CharType const* cstr) noexcept
        {
            return ice::string::push_back(str, ice::BasicString<CharType>{ cstr });
        }

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, ice::HeapString<CharType> const& other) noexcept
        {
            return ice::string::push_back(str, other.operator ice::BasicString<CharType>());
        }

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, ice::BasicString<CharType> other) noexcept
        {
            if (ice::string::empty(other) == false)
            {
                uint32_t const new_size = str._size + ice::string::size(other);
                if (new_size + 1 > str._capacity)
                {
                    ice::string::grow(str, new_size + 1);
                }

                ice::memcpy(ice::string::end(str), ice::string::data(other), ice::string::size(other) * sizeof(CharType));
                str._size = new_size;
                str._data[str._size] = 0;
            }
        }

        template<typename CharType>
        inline void pop_back(ice::HeapString<CharType>& str, uint32_t count) noexcept
        {
            if (str._data != nullptr)
            {
                str._size -= ice::min(str._size, count);
                str._data[str._size] = CharType{ 0 };
            }
        }

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::iterator
        {
            return str._data;
        }

        template<typename CharType>
        inline auto end(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::iterator
        {
            return str._data + str._size;
        }

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data + str._size };
        }

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::reverse_iterator
        {
            return typename ice::HeapString<CharType>::reverse_iterator{ str._data };
        }


        template<typename CharType>
        inline auto size(ice::HeapString<CharType> const& str) noexcept -> uint32_t
        {
            return str._size;
        }

        template<typename CharType>
        inline auto length(ice::HeapString<CharType> const& str) noexcept -> uint32_t
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

        template<typename CharType>
        inline auto data(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type const*
        {
            return str._data;
        }

        template<typename CharType>
        inline auto capacity(ice::HeapString<CharType> const& str) noexcept -> uint32_t
        {
            return str._capacity;
        }

        template<typename CharType>
        inline bool empty(ice::HeapString<CharType> const& str) noexcept
        {
            return str._size == 0;
        }

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_iterator
        {
            return str._data;
        }

        template<typename CharType>
        inline auto end(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_iterator
        {
            return str._data + str._size;
        }

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_reverse_iterator
        {
            return typename ice::HeapString<CharType>::const_reverse_iterator{ str._data + str._size };
        }

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::const_reverse_iterator
        {
            return typename ice::HeapString<CharType>::const_reverse_iterator{ str._data };
        }

        template<typename CharType>
        inline auto front(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type
        {
            return str._data[0];
        }

        template<typename CharType>
        inline auto back(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::value_type
        {
            return str._data[str._size - 1];
        }

        template<typename CharType>
        inline auto substr(ice::HeapString<CharType> const& str, uint32_t pos, uint32_t len) noexcept -> ice::BasicString<CharType>
        {
            return ice::string::substr(ice::BasicString<CharType>{ str }, pos, len);
        }

        template<typename CharType>
        inline auto substr_clone(ice::HeapString<CharType> const& str, uint32_t pos, uint32_t len) noexcept -> ice::HeapString<CharType>
        {
            if (pos >= str._size)
            {
                return ice::HeapString<CharType>{ *str._allocator };
            }

            uint32_t pos_end = str._size;
            if (len != ice::string_npos)
            {
                pos_end = ice::min(pos_end, pos + len);
            }

            return ice::HeapString<CharType>{
                *str._allocator,
                ice::String{ str._data + pos, str._data + pos_end }
            };
        }

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto it = ice::string::begin(str);
            auto const it_end = ice::string::end(str);

            while(it != it_end && *it != character_value)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : result;
        }

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
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

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && *it != character_value)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : (str._size - result - 1);
        }

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
        {
            uint32_t result = 0;

            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::string_npos)
            {
                it += 1;
                result += 1;
            }

            return it == it_end ? ice::string_npos : (str._size - result - 1);
        }

        template<typename CharType>
        inline bool equals(ice::HeapString<CharType> const& left, ice::HeapString<CharType> const& right) noexcept
        {
            return ice::string::equals(ice::BasicString<CharType>{ left }, ice::BasicString<CharType>{ right });
        }

    } // namespace string

} // namespace ice
