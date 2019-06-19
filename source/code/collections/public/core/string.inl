
// core::String methods
//////////////////////////////////////////////////////////////////////////

template <typename CharType>
inline core::String<CharType>::String(core::allocator& allocator) noexcept
    : _allocator{ &allocator }
{
}

template <typename CharType>
inline core::String<CharType>::String(const String<CharType>& other) noexcept
    : _allocator{ other._allocator }
{
    const uint32_t n = other._size + 1;
    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
}

template <typename CharType>
inline core::String<CharType>::~String() noexcept
{
    _allocator->deallocate(_data);
}

template <typename CharType>
inline auto core::String<CharType>::operator=(const String<CharType>& other) noexcept -> String<CharType>&
{
    const uint32_t n = other._size;
    string::resize(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * (n + 1));
    return *this;
}

template <typename CharType>
template<uint32_t OtherCapacity>
inline auto core::String<CharType>::operator=(const StackString<OtherCapacity, CharType>& other) noexcept -> String<CharType>&
{
    *this = other._data;
    return *this;
}

template <typename CharType>
inline auto core::String<CharType>::operator=(const CharType* other) noexcept -> String<CharType>&
{
    const auto n = strlen(other);
    string::resize(*this, static_cast<uint32_t>(n));
    memcpy(_data, other, sizeof(CharType) * (n + 1));
    return *this;
}

template <typename CharType>
inline auto core::String<CharType>::operator[](uint32_t i) noexcept -> CharType&
{
    return _data[i];
}

template <typename CharType>
inline auto core::String<CharType>::operator[](uint32_t i) const noexcept -> const CharType&
{
    return _data[i];
}

template<typename CharType>
inline auto core::begin(String<CharType>& a) noexcept -> CharType*
{
    return string::begin(a);
}

template<typename CharType>
inline auto core::begin(const String<CharType>& a) noexcept -> const CharType*
{
    return string::begin(a);
}

template<typename CharType>
inline auto core::end(String<CharType>& a) noexcept -> CharType*
{
    return string::end(a);
}

template<typename CharType>
inline auto core::end(const String<CharType>& a) noexcept -> const CharType*
{
    return string::end(a);
}

template<typename CharType>
inline void core::swap(String<CharType>& lhs, String<CharType>& rhs) noexcept
{
    std::swap(lhs._allocator, rhs._allocator);
    std::swap(lhs._size, rhs._size);
    std::swap(lhs._capacity, rhs._capacity);
    std::swap(lhs._data, rhs._data);
}

// core::String functions
//////////////////////////////////////////////////////////////////////////


template<typename CharType>
inline auto core::string::size(const core::String<CharType>& str) noexcept -> uint32_t
{
    return str._size == 0 ? str._size : str._size + 1;
}

template<typename CharType>
inline auto core::string::capacity(const core::String<CharType>& str) noexcept -> uint32_t
{
    return str._capacity;
}

template<typename CharType>
inline auto core::string::length(const String<CharType>& a) noexcept -> uint32_t
{
    return a._size;
}

template<typename CharType>
inline bool core::string::empty(const String<CharType>& a) noexcept
{
    return a._size == 0;
}

template<typename CharType>
inline auto core::string::begin(String<CharType>& a) noexcept -> CharType*
{
    return a._data;
}

template<typename CharType>
inline auto core::string::begin(const String<CharType>& a) noexcept -> const CharType*
{
    return a._data;
}

template<typename CharType>
inline auto core::string::end(String<CharType>& a) noexcept -> CharType*
{
    return a._data + a._size;
}

template<typename CharType>
inline auto core::string::end(const String<CharType>& a) noexcept -> const CharType*
{
    return a._data + a._size;
}

template<typename CharType>
inline auto core::string::front(String<CharType>& a) noexcept -> CharType&
{
    return a._data[0];
}

template<typename CharType>
inline auto core::string::front(const String<CharType>& a) noexcept -> const CharType&
{
    return a._data[0];
}

template<typename CharType>
inline auto core::string::back(String<CharType>& a) noexcept -> CharType&
{
    return a._data[a._size - 1];
}

template<typename CharType>
inline auto core::string::back(const String<CharType>& a) noexcept -> const CharType&
{
    return a._data[a._size - 1];
}


template <typename CharType>
inline void core::string::clear(String<CharType>& a) noexcept
{
    resize(a, 0);
}

template <typename CharType>
inline void core::string::trim(String<CharType>& a) noexcept
{
    set_capacity(a, a._size + 1);
}

template <typename CharType>
inline void core::string::resize(String<CharType>& a, uint32_t new_size) noexcept
{
    if (new_size + 1 > a._capacity)
        grow(a, new_size + 1);
    a._size = new_size;
    a._data[a._size] = 0;
}

template <typename CharType>
inline void core::string::reserve(String<CharType>& a, uint32_t new_capacity) noexcept
{
    if (new_capacity > a._capacity)
        set_capacity(a, new_capacity);
}

template<typename CharType>
inline void core::string::set_capacity(String<CharType>& a, uint32_t new_capacity) noexcept
{
    if (new_capacity == a._capacity)
        return;

    if (new_capacity < a._size)
        a._size = new_capacity - 1;

    CharType* new_data = 0;
    if (new_capacity > 0)
    {
        new_data = reinterpret_cast<CharType*>(a._allocator->allocate(sizeof(CharType) * new_capacity, alignof(CharType)));
        memcpy(new_data, a._data, sizeof(CharType) * a._size);
    }
    else if (new_capacity == 0)
    {
        a._size = 0;
    }
    a._allocator->deallocate(a._data);
    a._data = new_data;
    a._capacity = new_capacity;
}

template<typename CharType>
inline void core::string::grow(String<CharType>& a, uint32_t min_capacity) noexcept
{
    uint32_t new_capacity = a._capacity * 2 + 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;
    set_capacity(a, new_capacity);
}

template<typename CharType>
inline void core::string::push_back(String<CharType>& a, const CharType& item) noexcept
{
    if (a._size + 1 > a._capacity)
        grow(a);
    a._data[a._size] = item;
    a._data[++a._size] = 0;
}

template<typename CharType>
inline void core::string::push_back(String<CharType>& a, const CharType* character_array) noexcept
{
    auto str_len = strlen(character_array);
    if (str_len > 0)
    {
        auto new_size = a._size + str_len;
        if (new_size + 1 > a._capacity)
            grow(a, static_cast<uint32_t>(new_size) + 1);
        memcpy(string::end(a), character_array, str_len);
        a._size = static_cast<uint32_t>(new_size);
        a._data[a._size] = 0;
    }
}

template<typename CharType>
inline void core::string::pop_back(String<CharType> & a) noexcept
{
    a._data[--a._size] = 0;
}

template<typename CharType>
inline void core::string::pop_back(String<CharType>& a, uint32_t num) noexcept
{
    if (a._size < num)
        a._size = num;
    a._size -= num;
    a._data[a._size] = 0;
}

// core::String operators
//////////////////////////////////////////////////////////////////////////
