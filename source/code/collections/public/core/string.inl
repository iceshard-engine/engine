
// core::String methods
//////////////////////////////////////////////////////////////////////////

template <typename CharType>
inline core::String<CharType>::String(core::allocator& allocator) noexcept
    : _allocator{ &allocator }
{
}

template <typename CharType>
inline core::String<CharType>::String(core::allocator& allocator, const CharType* value) noexcept
    : _allocator{ &allocator }
{
    *this = value;
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
inline core::String<CharType>::String(String<CharType>&& other) noexcept
    : _allocator{ other._allocator }
    , _size{ other._size }
    , _capacity{ other._capacity }
    , _data{ other._data }
{
    other._data = nullptr;
    other._capacity = 0;
    other._size = 0;
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
inline auto core::String<CharType>::operator=(String<CharType>&& other) noexcept -> String<CharType>&
{
    if (this == &other)
    {
        return *this;
    }
    std::swap(_allocator, other._allocator);
    std::swap(_size, other._size);
    std::swap(_capacity, other._capacity);
    std::swap(_data, other._data);
    return *this;
}

template <typename CharType>
template<uint32_t Capacity>
inline auto core::String<CharType>::operator=(const StackString<Capacity, CharType>& other) noexcept -> String<CharType>&
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
inline auto core::begin(String<CharType>& str) noexcept -> CharType*
{
    return string::begin(str);
}

template<typename CharType>
inline auto core::begin(const String<CharType>& str) noexcept -> const CharType*
{
    return string::begin(str);
}

template<typename CharType>
inline auto core::end(String<CharType>& str) noexcept -> CharType*
{
    return string::end(str);
}

template<typename CharType>
inline auto core::end(const String<CharType>& str) noexcept -> const CharType*
{
    return string::end(str);
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
inline auto core::string::length(const String<CharType>& str) noexcept -> uint32_t
{
    return str._size;
}

template<typename CharType>
inline bool core::string::empty(const String<CharType>& str) noexcept
{
    return str._size == 0;
}

template<typename CharType>
inline auto core::string::begin(String<CharType>& str) noexcept -> CharType*
{
    return str._data;
}

template<typename CharType>
inline auto core::string::begin(const String<CharType>& str) noexcept -> const CharType*
{
    return str._data;
}

template<typename CharType>
inline auto core::string::end(String<CharType>& str) noexcept -> CharType*
{
    return str._data + str._size;
}

template<typename CharType>
inline auto core::string::end(const String<CharType>& str) noexcept -> const CharType*
{
    return str._data + str._size;
}

template<typename CharType>
inline auto core::string::front(String<CharType>& str) noexcept -> CharType&
{
    return str._data[0];
}

template<typename CharType>
inline auto core::string::front(const String<CharType>& str) noexcept -> const CharType&
{
    return str._data[0];
}

template<typename CharType>
inline auto core::string::back(String<CharType>& str) noexcept -> CharType&
{
    return str._data[str._size - 1];
}

template<typename CharType>
inline auto core::string::back(const String<CharType>& str) noexcept -> const CharType&
{
    return str._data[str._size - 1];
}


template <typename CharType>
inline void core::string::clear(String<CharType>& str) noexcept
{
    resize(str, 0);
}

template <typename CharType>
inline void core::string::trim(String<CharType>& str) noexcept
{
    set_capacity(str, str._size + 1);
}

template <typename CharType>
inline void core::string::resize(String<CharType>& str, uint32_t new_size) noexcept
{
    if (new_size + 1 > str._capacity)
    {
        grow(str, new_size + 1);
    }
    str._size = new_size;
    str._data[str._size] = 0;
}

template <typename CharType>
inline void core::string::reserve(String<CharType>& str, uint32_t new_capacity) noexcept
{
    if (new_capacity > str._capacity)
    {
        set_capacity(str, new_capacity);
    }
}

template<typename CharType>
inline void core::string::set_capacity(String<CharType>& str, uint32_t new_capacity) noexcept
{
    if (new_capacity == str._capacity)
    {
        return;
    }

    if (new_capacity < str._size)
    {
        str._size = new_capacity - 1;
    }

    CharType* new_data = 0;
    if (new_capacity > 0)
    {
        new_data = reinterpret_cast<CharType*>(str._allocator->allocate(sizeof(CharType) * new_capacity));
        memcpy(new_data, str._data, sizeof(CharType) * str._size);
        new_data[str._size] = 0;
    }
    else if (new_capacity == 0)
    {
        str._size = 0;
    }

    str._allocator->deallocate(str._data);
    str._data = new_data;
    str._capacity = new_capacity;
}

template<typename CharType>
inline void core::string::grow(String<CharType>& str, uint32_t min_capacity) noexcept
{
    uint32_t new_capacity = str._capacity * 2 + 8;
    if (new_capacity < min_capacity)
    {
        new_capacity = min_capacity;
    }

    set_capacity(str, new_capacity);
}

template<typename CharType>
inline void core::string::push_back(String<CharType>& str, CharType item) noexcept
{
    if (str._size + 1 > str._capacity)
    {
        grow(str);
    }

    str._data[str._size] = item;
    str._data[++str._size] = 0;
}

template<typename CharType>
inline void core::string::push_back(String<CharType>& str, const CharType* cstr) noexcept
{
    auto str_len = strlen(cstr);
    if (str_len > 0)
    {
        auto new_size = str._size + str_len;
        if (new_size + 1 > str._capacity)
        {
            grow(str, static_cast<uint32_t>(new_size) + 1);
        }

        memcpy(string::end(str), cstr, str_len);
        str._size = static_cast<uint32_t>(new_size);
        str._data[str._size] = 0;
    }
}

template<typename CharType>
inline void core::string::push_back(String<CharType>& str, const String<CharType>& other) noexcept
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

template<typename CharType>
inline void core::string::pop_back(String<CharType> & str) noexcept
{
    str._data[--str._size] = 0;
}

template<typename CharType>
inline void core::string::pop_back(String<CharType>& str, uint32_t num) noexcept
{
    if (str._size < num)
    {
        str._size = num;
    }

    str._size -= num;
    str._data[str._size] = 0;
}


// core::String functions
//////////////////////////////////////////////////////////////////////////


template <typename CharType>
bool core::string::equals(const String<CharType>& left, const String<CharType>& right) noexcept
{
    return equals(left, right._data);
}

template <typename CharType>
bool core::string::equals(const String<CharType>& left, const std::string_view right) noexcept
{
    return equals(left, right.data());
}

template <typename CharType>
bool core::string::equals(const String<CharType>& left, const CharType* right) noexcept
{
    return strcmp(left._data, right) == 0;
}


// core::String operators
//////////////////////////////////////////////////////////////////////////


template<typename CharType>
auto core::operator+=(String<CharType>& self, CharType other) noexcept -> String<CharType>&
{
    string::push_back(self, other);
    return self;
}

template<typename CharType>
auto core::operator+=(String<CharType>& self, const CharType* other) noexcept -> String<CharType>&
{
    string::push_back(self, other);
    return self;
}

template<typename CharType>
auto core::operator+=(String<CharType>& self, const String<CharType>& other) noexcept -> String<CharType>&
{
    string::push_back(self, other);
    return self;
}

template<uint32_t Capacity, typename CharType>
auto core::operator+=(String<CharType>& self, const StackString<Capacity, CharType>& other) noexcept -> String<CharType>&
{
    string::push_back(self, begin(other));
    return self;
}
