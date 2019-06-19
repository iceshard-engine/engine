

template <uint32_t Capacity, typename CharType>
inline auto core::string::size(const core::StackString<Capacity, CharType>& a) noexcept -> uint32_t
{
    return a._size + 1;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::capacity(const core::StackString<Capacity, CharType>&) noexcept -> uint32_t
{
    return Capacity;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::length(const core::StackString<Capacity, CharType>& a) noexcept -> uint32_t
{
    return a._size;
}

template <uint32_t Capacity, typename CharType>
inline bool core::string::empty(const core::StackString<Capacity, CharType>& a) noexcept
{
    return a._size == 0;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::begin(core::StackString<Capacity, CharType>& a) noexcept -> CharType*
{
    return a._data;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::begin(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType*
{
    return a._data;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::end(core::StackString<Capacity, CharType>& a) noexcept -> CharType*
{
    return a._data + a._size;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::end(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType*
{
    return a._data + a._size;
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::front(core::StackString<Capacity, CharType>& a) noexcept -> CharType&
{
    return a._data[0];
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::front(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType&
{
    return a._data[0];
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::back(core::StackString<Capacity, CharType>& a) noexcept -> CharType&
{
    return a._data[a._size - 1];
}

template <uint32_t Capacity, typename CharType>
inline auto core::string::back(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType&
{
    return a._data[a._size - 1];
}

template <uint32_t Capacity, typename CharType>
inline void core::string::clear(core::StackString<Capacity, CharType>& a) noexcept
{
    resize(a, 0);
}

template <uint32_t Capacity, typename CharType>
inline void core::string::trim(core::StackString<Capacity, CharType>& a) noexcept
{
    set_capacity(a, a._size + 1);
}

template <uint32_t Capacity, typename CharType>
inline void core::string::resize(core::StackString<Capacity, CharType>& a, uint32_t new_size) noexcept
{
    IS_ASSERT(new_size + 1 <= Capacity, "New size is larger than the available capacity! [ capacity={}, size={} ]", Capacity, new_size);
    a._size = new_size;
    a._data[a._size] = 0;
}

template <uint32_t Capacity, typename CharType>
inline void core::string::reserve(core::StackString<Capacity, CharType>&, uint32_t new_capacity) noexcept
{
    IS_ASSERT(new_capacity <= Capacity, "Requested capacity is larger than the available capacity! [ available={}, requested={} ]", Capacity, new_capacity);
}

template <uint32_t Capacity, typename CharType>
inline void core::string::set_capacity(core::StackString<Capacity, CharType>&, uint32_t new_capacity) noexcept
{
    IS_ASSERT(new_capacity <= Capacity, "Requested capacity is larger than the available capacity! [ available={}, requested={} ]", Capacity, new_capacity);
}

template <uint32_t Capacity, typename CharType>
inline void core::string::grow(core::StackString<Capacity, CharType>&, uint32_t min_capacity) noexcept
{
    IS_ASSERT(min_capacity <= Capacity, "Trying to grow over the available capacity! [ available={}, requested={} ]", Capacity, min_capacity);
}

template <uint32_t Capacity, typename CharType>
inline void core::string::push_back(core::StackString<Capacity, CharType>& a, CharType item) noexcept
{
    grow(a, a._size + 1);
    a._data[a._size] = item;
    a._data[a._size++] = 0;
}

template <uint32_t Capacity, typename CharType>
inline void core::string::push_back(StackString<Capacity, CharType>& a, const CharType* character_array) noexcept
{
    auto str_len = strlen(character_array);
    if (str_len > 0)
    {
        auto new_size = static_cast<uint32_t>(a._size + str_len);
        grow(a, new_size + 1);

        memcpy(string::end(a), character_array, str_len);
        a._size = new_size;
        a._data[a._size] = 0;
    }
}

template <uint32_t Capacity, typename CharType>
inline void core::string::push_back(StackString<Capacity, CharType>& str, const StackString<Capacity, CharType>& other) noexcept
{
    if (!string::empty(other))
    {
        // We need to reserve enough data for the concatenation, this will
        // allow us to handle the scenario when self appending. Because first
        // we reallocate the buffer if required and then we access it.
        string::reserve(str, string::size(str) + string::size(other) + 1);
        string::push_back(str, string::begin(other));
    }
}

template <uint32_t Capacity, typename CharType>
inline void core::string::pop_back(core::StackString<Capacity, CharType> & a) noexcept
{
    a._data[--a._size] = 0;
}

template <uint32_t Capacity, typename CharType>
inline void core::string::pop_back(core::StackString<Capacity, CharType>& a, uint32_t num) noexcept
{
    if (a._size < num)
        a._size = num;
    a._size -= num;
    a._data[a._size] = 0;
}


//////////////////////////////////////////////////////////////////////////


template <uint32_t Capacity, typename CharType>
inline core::StackString<Capacity, CharType>::StackString(const char* cstring) noexcept
    : StackString{ }
{
    *this = cstring;
}

template <uint32_t Capacity, typename CharType>
template <uint32_t OtherCapacity>
inline core::StackString<Capacity, CharType>::StackString(const core::StackString<OtherCapacity, CharType>& other) noexcept
    : StackString{ }
{
    static constexpr auto min_capacity = Capacity < OtherCapacity ? Capacity : OtherCapacity;

    const uint32_t n = min_capacity < other._size ? min_capacity : other._size;
    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
}

template <uint32_t Capacity, typename CharType>
template <uint32_t OtherCapacity>
inline auto core::StackString<Capacity, CharType>::operator=(const core::StackString<OtherCapacity, CharType>& other) noexcept -> core::StackString<Capacity, CharType>&
{
    static constexpr auto min_capacity = Capacity < OtherCapacity ? Capacity : OtherCapacity;
    static constexpr auto n = min_capacity <= other._size ? min_capacity - 1 : other._size;

    string::resize(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * (n + 1));
    return *this;
}

template<uint32_t Capacity, typename CharType>
inline auto core::StackString<Capacity, CharType>::operator=(const core::String<CharType>& other) noexcept -> core::StackString<Capacity, CharType>&
{
    static auto min_capacity = Capacity < other._capacity ? Capacity : other._capacity;
    static auto n = min_capacity < other._size ? Capacity : other._size;

    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
    return *this;
}

template <uint32_t Capacity, typename CharType>
inline auto core::StackString<Capacity, CharType>::operator=(const CharType* cstring) noexcept -> StackString<Capacity, CharType>&
{
    const auto str_len = strlen(cstring);
    const auto n = Capacity < str_len ? Capacity : str_len;

    _size = static_cast<uint32_t>(n);

    string::set_capacity(*this, _size);
    memcpy(_data, cstring, sizeof(CharType) * _size);
    return *this;
}

template <uint32_t Capacity, typename CharType>
inline auto core::StackString<Capacity, CharType>::operator[](uint32_t i) noexcept -> CharType&
{
    return _data[i];
}

template <uint32_t Capacity, typename CharType>
inline auto core::StackString<Capacity, CharType>::operator[](uint32_t i) const noexcept -> const CharType&
{
    return _data[i];
}

// range based ADL lookup

template <uint32_t Capacity, typename CharType>
inline auto core::begin(core::StackString<Capacity, CharType>& a) noexcept -> CharType*
{
    return string::begin(a);
}

template <uint32_t Capacity, typename CharType>
inline auto core::begin(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType*
{
    return string::begin(a);
}

template <uint32_t Capacity, typename CharType>
inline auto core::end(core::StackString<Capacity, CharType>& a) noexcept -> CharType*
{
    return string::end(a);
}

template <uint32_t Capacity, typename CharType>
inline auto core::end(const core::StackString<Capacity, CharType>& a) noexcept -> const CharType*
{
    return string::end(a);
}

template <uint32_t Capacity, typename CharType>
void core::swap(core::StackString<Capacity, CharType>& lhs, core::StackString<Capacity, CharType>& rhs) noexcept
{
    std::swap(lhs._allocator, rhs._allocator);
    std::swap(lhs._size, rhs._size);
    std::swap(lhs._capacity, rhs._capacity);
    std::swap(lhs._data, rhs._data);
}