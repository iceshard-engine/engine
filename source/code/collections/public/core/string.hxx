#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>

namespace core
{
namespace string
{

/// The number of characters in the string.
template <typename CharType> uint32_t size(const String<CharType>& a);
/// The length of the string (until the first null character).
template <typename CharType> uint32_t length(const String<CharType>& a);
/// Returns true if the string is empty.
template <typename CharType> bool empty(const String<CharType>& a);

/// Used to iterate over the string.
template <typename CharType> CharType* begin(String<CharType>& a);
template <typename CharType> const CharType* begin(const String<CharType>& a);
template <typename CharType> CharType* end(String<CharType>& a);
template <typename CharType> const CharType* end(const String<CharType>& a);

/// Returns the first/last element of the string. Don't use these on an
/// empty string.
template <typename CharType> CharType& front(String<CharType>& a);
template <typename CharType> const CharType& front(const String<CharType>& a);
template <typename CharType> CharType& back(String<CharType>& a);
template <typename CharType> const CharType& back(const String<CharType>& a);

/// Changes the size of the string (does not reallocate memory unless necessary).
template <typename CharType> void resize(String<CharType>& a, uint32_t new_size);
/// Removes all items in the string (does not free memory).
template <typename CharType> void clear(String<CharType>& a);
/// Reallocates the string to the specified capacity.
template <typename CharType> void set_capacity(String<CharType>& a, uint32_t new_capacity);
/// Makes sure that the string has at least the specified capacity.
/// (If not, the string is grown.)
template <typename CharType> void reserve(String<CharType>& a, uint32_t new_capacity);
/// Grows the string using a geometric progression formula, so that the amortized
/// cost of push_back() is O(1). If a min_capacity is specified, the string will
/// grow to at least that capacity.
template <typename CharType> void grow(String<CharType>& a, uint32_t min_capacity = 0);
/// Trims the string so that its capacity matches its size.
template <typename CharType> void trim(String<CharType>& a);

/// Pushes the character to the end of the string.
template <typename CharType> void push_back(String<CharType>& a, const CharType& character);
/// Pushes the character array to the end of the string.
template <typename CharType> void push_back(String<CharType>& a, const CharType* character_array);
/// Pops the last character from the string. The string cannot be empty.
template <typename CharType> void pop_back(String<CharType>& a);
/// Pops the last N character from the string. The string cannot be empty.
template <typename CharType> void pop_back(String<CharType>& a, uint32_t num);
}

namespace string
{
template<typename CharType> inline uint32_t size(const String<CharType>& a) { return a._size == 0 ? a._size : a._size + 1; }
template<typename CharType> inline uint32_t length(const String<CharType>& a) { return a._size; }
template<typename CharType> inline bool any(const String<CharType>& a) { return a._size != 0; }
template<typename CharType> inline bool empty(const String<CharType>& a) { return a._size == 0; }

template<typename CharType> inline CharType* begin(String<CharType>& a) { return a._data; }
template<typename CharType> inline const CharType* begin(const String<CharType>& a) { return a._data; }
template<typename CharType> inline CharType* end(String<CharType>& a) { return a._data + a._size; }
template<typename CharType> inline const CharType* end(const String<CharType>& a) { return a._data + a._size; }

template<typename CharType> inline CharType& front(String<CharType>& a) { return a._data[0]; }
template<typename CharType> inline const CharType& front(const String<CharType>& a) { return a._data[0]; }
template<typename CharType> inline CharType& back(String<CharType>& a) { return a._data[a._size - 1]; }
template<typename CharType> inline const CharType& back(const String<CharType>& a) { return a._data[a._size - 1]; }

template <typename CharType> inline void clear(String<CharType>& a) { resize(a, 0); }
template <typename CharType> inline void trim(String<CharType>& a) { set_capacity(a, a._size + 1); }

template <typename CharType> void resize(String<CharType>& a, uint32_t new_size)
{
    if (new_size + 1 > a._capacity)
        grow(a, new_size + 1);
    a._size = new_size;
    a._data[a._size] = 0;
}

template <typename CharType> inline void reserve(String<CharType>& a, uint32_t new_capacity)
{
    if (new_capacity > a._capacity)
        set_capacity(a, new_capacity);
}

template<typename CharType> void set_capacity(String<CharType>& a, uint32_t new_capacity)
{
    if (new_capacity == a._capacity)
        return;

    if (new_capacity < a._size)
        a._size = new_capacity;

    CharType * new_data = 0;
    if (new_capacity > 0) {
        new_data = (CharType*)a._allocator->allocate(sizeof(CharType) * new_capacity, alignof(CharType));
        memcpy(new_data, a._data, sizeof(CharType) * a._size);
    }
    a._allocator->deallocate(a._data);
    a._data = new_data;
    a._capacity = new_capacity;
}

template<typename CharType> void grow(String<CharType>& a, uint32_t min_capacity)
{
    uint32_t new_capacity = a._capacity * 2 + 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;
    set_capacity(a, new_capacity);
}

template<typename CharType> inline void push_back(String<CharType>& a, const CharType& item)
{
    if (a._size + 1 > a._capacity)
        grow(a);
    a._data[a._size] = item;
    a._data[a._size++] = 0;
}

template<typename CharType> inline void push_back(String<CharType>& a, const CharType* character_array)
{
    auto str_len = strlen(character_array);
    if (str_len > 0)
    {
        auto new_size = a._size + str_len;
        if (new_size + 1 > a._capacity)
            grow(a, new_size + 1);
        memcpy(string::end(a), character_array, str_len);
        a._size = new_size;
        a._data[a._size] = 0;
    }
}

template<typename CharType> inline void pop_back(String<CharType> & a)
{
    a._data[--a._size] = 0;
}

template<typename CharType>
void pop_back(String<CharType>& a, uint32_t num)
{
    if (a._size < num)
        a._size = num;
    a._size -= num;
    a._data[a._size] = 0;
}

}

template <typename CharType>
inline String<CharType>::String(core::allocator& allocator) : _allocator(&allocator), _size(0), _capacity(0), _data(0) {}

template <typename CharType>
inline String<CharType>::~String()
{
    _allocator->deallocate(_data);
}

template <typename CharType>
inline String<CharType>::String(const String<CharType>& other) : _allocator(other._allocator), _size(0), _capacity(0), _data(0)
{
    const uint32_t n = other._size + 1;
    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
}

template <typename CharType>
inline String<CharType>& String<CharType>::operator=(const String<CharType>& other)
{
    const uint32_t n = other._size;
    string::resize(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * (n + 1));
    return *this;
}

template <typename CharType>
template<uint32_t OtherCapacity>
inline String<CharType>& String<CharType>::operator=(const StackString<OtherCapacity, CharType>& other)
{
    *this = other._data;
    return *this;
}

template <typename CharType>
inline String<CharType>& String<CharType>::operator=(const CharType* other)
{
    const auto n = strlen(other);
    string::resize(*this, static_cast<uint32_t>(n));
    memcpy(_data, other, sizeof(CharType) * (n + 1));
    return *this;
}

template <typename CharType>
inline CharType& String<CharType>::operator[](uint32_t i)
{
    return _data[i];
}

template <typename CharType>
inline const CharType& String<CharType>::operator[](uint32_t i) const
{
    return _data[i];
}

// range based ADL lookup
template<typename CharType> inline CharType* begin(String<CharType>& a)
{
    return string::begin(a);
}

template<typename CharType> inline const CharType* begin(const String<CharType>& a)
{
    return string::begin(a);
}

template<typename CharType> inline CharType* end(String<CharType>& a)
{
    return string::end(a);
}

template<typename CharType> inline const CharType* end(const String<CharType>& a)
{
    return string::end(a);
}

template<typename CharType> void swap(String<CharType>& lhs, String<CharType>& rhs)
{
    std::swap(lhs._allocator, rhs._allocator);
    std::swap(lhs._size, rhs._size);
    std::swap(lhs._capacity, rhs._capacity);
    std::swap(lhs._data, rhs._data);
}

} // namespace core::string
