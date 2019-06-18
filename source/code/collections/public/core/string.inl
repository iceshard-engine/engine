
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

template<typename CharType> inline uint32_t core::string::length(const String<CharType>& a) noexcept { return a._size; }
//template<typename CharType> inline bool core::string::any(const String<CharType>& a) { return a._size != 0; }
template<typename CharType> inline bool core::string::empty(const String<CharType>& a) noexcept { return a._size == 0; }

template<typename CharType> inline CharType* core::string::begin(String<CharType>& a) noexcept { return a._data; }
template<typename CharType> inline const CharType* core::string::begin(const String<CharType>& a) noexcept { return a._data; }
template<typename CharType> inline CharType* core::string::end(String<CharType>& a) noexcept { return a._data + a._size; }
template<typename CharType> inline const CharType* core::string::end(const String<CharType>& a) noexcept { return a._data + a._size; }

template<typename CharType> inline CharType& core::string::front(String<CharType>& a) noexcept { return a._data[0]; }
template<typename CharType> inline const CharType& core::string::front(const String<CharType>& a) noexcept { return a._data[0]; }
template<typename CharType> inline CharType& core::string::back(String<CharType>& a) noexcept { return a._data[a._size - 1]; }
template<typename CharType> inline const CharType& core::string::back(const String<CharType>& a) noexcept { return a._data[a._size - 1]; }

template <typename CharType> inline void core::string::clear(String<CharType>& a) noexcept { resize(a, 0); }
template <typename CharType> inline void core::string::trim(String<CharType>& a) noexcept { set_capacity(a, a._size + 1); }

template <typename CharType> void core::string::resize(String<CharType>& a, uint32_t new_size) noexcept
{
    if (new_size + 1 > a._capacity)
        grow(a, new_size + 1);
    a._size = new_size;
    a._data[a._size] = 0;
}

template <typename CharType> inline void core::string::reserve(String<CharType>& a, uint32_t new_capacity) noexcept
{
    if (new_capacity > a._capacity)
        set_capacity(a, new_capacity);
}

template<typename CharType> void core::string::set_capacity(String<CharType>& a, uint32_t new_capacity) noexcept
{
    if (new_capacity == a._capacity)
        return;

    if (new_capacity < a._size)
        a._size = new_capacity - 1;

    CharType * new_data = 0;
    if (new_capacity > 0)
    {
        new_data = (CharType*)a._allocator->allocate(sizeof(CharType) * new_capacity, alignof(CharType));
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

template<typename CharType> void core::string::grow(String<CharType>& a, uint32_t min_capacity) noexcept
{
    uint32_t new_capacity = a._capacity * 2 + 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;
    set_capacity(a, new_capacity);
}

template<typename CharType> inline void core::string::push_back(String<CharType>& a, const CharType& item) noexcept
{
    if (a._size + 1 > a._capacity)
        grow(a);
    a._data[a._size] = item;
    a._data[++a._size] = 0;
}

template<typename CharType> inline void core::string::push_back(String<CharType>& a, const CharType* character_array) noexcept
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

template<typename CharType> inline void core::string::pop_back(String<CharType> & a) noexcept
{
    a._data[--a._size] = 0;
}

template<typename CharType>
void core::string::pop_back(String<CharType>& a, uint32_t num) noexcept
{
    if (a._size < num)
        a._size = num;
    a._size -= num;
    a._data[a._size] = 0;
}