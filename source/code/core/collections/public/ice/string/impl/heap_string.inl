
namespace ice
{

    template<typename CharType>
    inline HeapString<CharType>::HeapString(ice::Allocator& allocator) noexcept
        : _allocator{ &allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(ice::Allocator& allocator, ice::BasicString<CharType> value) noexcept
        : _allocator{ &allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
        *this = value;
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(HeapString<CharType>&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ ice::exchange(other._size, 0) }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename CharType>
    inline HeapString<CharType>::HeapString(HeapString<CharType> const& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
        if (other._size > 0)
        {
            ice::string::set_capacity(*this, other._size);

            ice::memcpy(
                ice::string::memory(*this),
                ice::string::data_view(other)
            );

            _size = other._size;
            _data[_size] = CharType{ 0 };
        }
    }

    template<typename CharType>
    inline HeapString<CharType>::~HeapString() noexcept
    {
        _allocator->deallocate(ice::string::memory(*this));
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(HeapString<CharType>&& other) noexcept -> HeapString<CharType>&
    {
        if (this != &other)
        {
            ice::string::set_capacity(*this, 0);

            _allocator = other._allocator;
            _size = ice::exchange(other._size, 0);
            _capacity = ice::exchange(other._capacity, 0);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(HeapString<CharType> const& other) noexcept -> HeapString<CharType>&
    {
        if (this != &other)
        {
            ice::string::clear(*this);
            ice::string::reserve(*this, other._capacity);

            if (other._size > 0)
            {
                ice::memcpy(
                    ice::string::memory(*this),
                    ice::string::data_view(other)
                );
            }

            _size= other._size;
            _data[_size] = CharType{ };
        }
        return *this;
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator=(ice::BasicString<CharType> str) noexcept -> HeapString<CharType>&
    {
        auto const* const other_str_begin = ice::string::begin(str);
        bool const part_of_this = other_str_begin >= ice::string::begin(*this)
            && other_str_begin < ice::string::end(*this);

        if (!part_of_this)
        {
            ice::string::set_capacity(
                *this,
                ice::string::size(str) + 1
            );

            ice::memcpy(
                ice::string::memory(*this),
                ice::string::data_view(str)
            );

            _size = ice::string::size(str);
            _data[_size] = ValueType{ 0 };
        }
        return *this;
    }

    template<typename CharType>
    inline bool operator==(ice::HeapString<CharType> const& left, CharType const* right) noexcept
    {
        return ice::BasicString<CharType>{ left } == ice::BasicString<CharType>{ right };
    }

    template<typename CharType>
    inline bool operator==(ice::HeapString<CharType> const& left, ice::BasicString<CharType> right) noexcept
    {
        return ice::BasicString<CharType>{ left } == right;
    }

    template<typename CharType>
    inline bool operator==(ice::BasicString<CharType> left, ice::HeapString<CharType> const& right) noexcept
    {
        return left == ice::BasicString<CharType>{ right };
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator[](ice::ucount index) noexcept -> CharType&
    {
        return _data[index];
    }

    template<typename CharType>
    inline auto HeapString<CharType>::operator[](ice::ucount index) const noexcept -> CharType const&
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
        inline void set_capacity(ice::HeapString<CharType>& str, ice::ucount new_capacity) noexcept
        {
            using ValueType = typename HeapString<CharType>::ValueType;

            if (new_capacity == str._capacity)
            {
                return;
            }

            ValueType* new_data = nullptr;
            if (new_capacity > 0)
            {
                ice::AllocResult new_buffer = str._allocator->allocate(ice::meminfo_of<ValueType> * new_capacity);

                if (str._size > 0)
                {
                    ice::memcpy(new_buffer, ice::string::data_view(str));
                }

                new_data = reinterpret_cast<ValueType*>(new_buffer.memory);
            }

            str._allocator->deallocate(ice::string::memory(str));
            str._capacity = new_capacity;
            str._data = new_data;

            if (new_capacity <= str._size)
            {
                str._size = ice::min(new_capacity, new_capacity - 1);
            }

            if (str._data != nullptr)
            {
                str._data[str._size] = 0;
            }
        }

        template<typename CharType>
        inline void reserve(ice::HeapString<CharType>& str, ice::ucount min_capacity) noexcept
        {
            if (min_capacity > str._capacity)
            {
                ice::string::set_capacity(str, min_capacity);
            }
        }

        template<typename CharType>
        inline void grow(ice::HeapString<CharType>& str, ice::ucount min_capacity) noexcept
        {
            ice::ucount const new_capacity = ice::max(str._capacity * 2 + 8, min_capacity);
            ice::string::set_capacity(str, new_capacity);
        }

        template<typename CharType>
        inline void resize(ice::HeapString<CharType>& str, ice::ucount new_size) noexcept
        {
            if (new_size > (str._capacity - 1))
            {
                ice::string::set_capacity(str, new_size + 1);
            }

            str._size = new_size;
            if (str._data != nullptr)
            {
                str._data[str._size] = CharType{ 0 };
            }
        }

        template<typename CharType>
        inline void shrink(ice::HeapString<CharType>& str) noexcept
        {
            ice::string::set_capacity(str, str._size + 1);
        }

        template<typename CharType>
        inline void clear(ice::HeapString<CharType>& str) noexcept
        {
            ice::string::resize(str, 0);
        }

        template<typename CharType>
        inline void push_back(ice::HeapString<CharType>& str, CharType character) noexcept
        {
            if (str._size + 1 >= str._capacity)
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
                if (new_size + 1 >= str._capacity)
                {
                    ice::string::grow(str, new_size + 1);
                }

                ice::memcpy(
                    ice::string::memory(str),
                    ice::string::data_view(other)
                );
                str._size = new_size;
                str._data[str._size] = 0;
            }
        }

        template<typename CharType>
        inline void pop_back(ice::HeapString<CharType>& str, ice::ucount count) noexcept
        {
            if (str._data != nullptr)
            {
                str._size -= ice::min(str._size, count);
                str._data[str._size] = CharType{ 0 };
            }
        }

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::Iterator
        {
            return str._data;
        }

        template<typename CharType>
        inline auto end(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::Iterator
        {
            return str._data + str._size;
        }

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::ReverseIterator
        {
            return typename ice::HeapString<CharType>::ReverseIterator{ str._data + str._size };
        }

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType>& str) noexcept -> typename ice::HeapString<CharType>::ReverseIterator
        {
            return typename ice::HeapString<CharType>::ReverseIterator{ str._data };
        }


        template<typename CharType>
        inline auto size(ice::HeapString<CharType> const& str) noexcept -> ice::ucount
        {
            return str._size;
        }

        template<typename CharType>
        inline auto capacity(ice::HeapString<CharType> const& str) noexcept -> ice::ucount
        {
            return str._capacity;
        }

        template<typename CharType>
        inline bool empty(ice::HeapString<CharType> const& str) noexcept
        {
            return str._size == 0;
        }

        template<typename CharType>
        inline auto begin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstIterator
        {
            return str._data;
        }

        template<typename CharType>
        inline auto end(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstIterator
        {
            return str._data + str._size;
        }

        template<typename CharType>
        inline auto rbegin(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstReverseIterator
        {
            return typename ice::HeapString<CharType>::ConstReverseIterator{ str._data + str._size };
        }

        template<typename CharType>
        inline auto rend(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ConstReverseIterator
        {
            return typename ice::HeapString<CharType>::ConstReverseIterator{ str._data };
        }

        template<typename CharType>
        inline auto front(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ValueType
        {
            return str._data[0];
        }

        template<typename CharType>
        inline auto back(ice::HeapString<CharType> const& str) noexcept -> typename ice::HeapString<CharType>::ValueType
        {
            return str._data[str._size - 1];
        }

        template<typename CharType>
        inline auto substr(ice::HeapString<CharType> const& str, ice::ucount pos, ice::ucount len) noexcept -> ice::BasicString<CharType>
        {
            return ice::string::substr(ice::BasicString<CharType>{ str }, pos, len);
        }

        template<typename CharType>
        inline auto substr_clone(ice::HeapString<CharType> const& str, ice::ucount pos, ice::ucount len) noexcept -> ice::HeapString<CharType>
        {
            if (pos >= str._size)
            {
                return ice::HeapString<CharType>{ *str._allocator };
            }

            ice::ucount pos_end = str._size;
            if (len != ice::String_NPos)
            {
                pos_end = ice::min(pos_end, pos + len);
            }

            return ice::HeapString<CharType>{
                *str._allocator,
                ice::BasicString<CharType>{ str._data + pos, str._data + pos_end }
            };
        }

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            auto const* it = ice::string::begin(str);
            auto const* const end = ice::string::end(str);

            while (it != end && *it != character_value)
            {
                it += 1;
            }

            return it == end ? ice::String_NPos : end - it;
        }

        template<typename CharType>
        inline auto find_first_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
        {
            auto const* it = ice::string::begin(str);
            auto const* const it_end = ice::string::end(str);

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::String_NPos)
            {
                it += 1;
            }

            return it == it_end ? ice::String_NPos : it_end - it;
        }

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, CharType character_value) noexcept -> uint32_t
        {
            auto const* it = ice::string::rbegin(str);
            auto const* const end = ice::string::rend(str);

            while (it != end && *it != character_value)
            {
                it += 1;
            }

            return it == end ? ice::String_NPos : end - it;
        }

        template<typename CharType>
        inline auto find_last_of(ice::HeapString<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> uint32_t
        {
            auto const* it = ice::string::rbegin(str);
            auto const* const it_end = ice::string::rend(str);

            while (it != it_end && ice::string::find_first_of(character_values, *it) == ice::String_NPos)
            {
                it += 1;
            }

            return it == it_end ? ice::String_NPos : it_end - it;
        }

        template<typename CharType>
        inline bool equals(ice::HeapString<CharType> const& left, ice::HeapString<CharType> const& right) noexcept
        {
            return ice::string::equals(ice::BasicString<CharType>{ left }, ice::BasicString<CharType>{ right });
        }


        template<typename CharType>
        inline auto data_view(ice::HeapString<CharType> const& str) noexcept -> ice::Data
        {
            return ice::Data{
                .location = str._data,
                .size = str._size,
                .alignment = ice::align_of<CharType>
            };
        }

        template<typename CharType>
        inline auto memory(ice::HeapString<CharType>& str) noexcept -> ice::Memory
        {
            return ice::Memory{
                .location = str._data,
                .size = str._capacity,
                .alignment = ice::align_of<CharType>
            };
        }

    } // namespace string

} // namespace ice
