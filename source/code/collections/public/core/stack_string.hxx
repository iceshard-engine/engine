#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>
#include <core/debug/assert.hxx>
#include <fmt/format.h>

namespace core
{

//! \brief A deduction guide.
template<uint32_t Size, typename CharType>
StackString(const CharType(&)[Size]) -> StackString<core::cexpr::roundup(Size), CharType>;

namespace string
{

/// The number of characters in the string.
template <uint32_t Capacity, typename CharType> uint32_t size(const StackString<Capacity, CharType>& a);
/// The length of the string (until the first null character).
template <uint32_t Capacity, typename CharType> uint32_t length(const StackString<Capacity, CharType>& a);
/// Returns true if the string is empty.
template <uint32_t Capacity, typename CharType> bool empty(const StackString<Capacity, CharType>& a);

/// Used to iterate over the string.
template <uint32_t Capacity, typename CharType> CharType* begin(StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> const CharType* begin(const StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> CharType* end(StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> const CharType* end(const StackString<Capacity, CharType>& a);

/// Returns the first/last element of the string. Don't use these on an
/// empty string.
template <uint32_t Capacity, typename CharType> CharType& front(StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> const CharType& front(const StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> CharType& back(StackString<Capacity, CharType>& a);
template <uint32_t Capacity, typename CharType> const CharType& back(const StackString<Capacity, CharType>& a);

/// Changes the size of the string (does not reallocate memory unless necessary).
template <uint32_t Capacity, typename CharType> void resize(StackString<Capacity, CharType>& a, uint32_t new_size);
/// Removes all items in the string (does not free memory).
template <uint32_t Capacity, typename CharType> void clear(StackString<Capacity, CharType>& a);
/// Reallocates the string to the specified capacity.
template <uint32_t Capacity, typename CharType> void set_capacity(StackString<Capacity, CharType>& a, uint32_t new_capacity);
/// Makes sure that the string has at least the specified capacity.
/// (If not, the string is grown.)
template <uint32_t Capacity, typename CharType> void reserve(StackString<Capacity, CharType>& a, uint32_t new_capacity);
/// Grows the string using a geometric progression formula, so that the amortized
/// cost of push_back() is O(1). If a min_capacity is specified, the string will
/// grow to at least that capacity.
template <uint32_t Capacity, typename CharType> void grow(StackString<Capacity, CharType>& a, uint32_t min_capacity = 0);
/// Trims the string so that its capacity matches its size.
template <uint32_t Capacity, typename CharType> void trim(StackString<Capacity, CharType>& a);

/// Pushes the character to the end of the string.
template <uint32_t Capacity, typename CharType> void push_back(StackString<Capacity, CharType>& a, const CharType& character);
/// Pushes the character array to the end of the string.
template <uint32_t Capacity, typename CharType> void push_back(StackString<Capacity, CharType>& a, const CharType* character_array);
/// Pops the last character from the string. The string cannot be empty.
template <uint32_t Capacity, typename CharType> void pop_back(StackString<Capacity, CharType>& a);
/// Pops the last N character from the string. The string cannot be empty.
template <uint32_t Capacity, typename CharType> void pop_back(StackString<Capacity, CharType>& a, uint32_t num);
}

namespace string
{
template <uint32_t Capacity, typename CharType> inline uint32_t size(const StackString<Capacity, CharType>& a) { return a._size + 1; }
template <uint32_t Capacity, typename CharType> inline uint32_t length(const StackString<Capacity, CharType>& a) { return a._size; }
template <uint32_t Capacity, typename CharType> inline bool any(const StackString<Capacity, CharType>& a) { return a._size != 0; }
template <uint32_t Capacity, typename CharType> inline bool empty(const StackString<Capacity, CharType>& a) { return a._size == 0; }

template <uint32_t Capacity, typename CharType> inline CharType* begin(StackString<Capacity, CharType>& a) { return a._data; }
template <uint32_t Capacity, typename CharType> inline const CharType* begin(const StackString<Capacity, CharType>& a) { return a._data; }
template <uint32_t Capacity, typename CharType> inline CharType* end(StackString<Capacity, CharType>& a) { return a._data + a._size; }
template <uint32_t Capacity, typename CharType> inline const CharType* end(const StackString<Capacity, CharType>& a) { return a._data + a._size; }

template <uint32_t Capacity, typename CharType> inline CharType& front(StackString<Capacity, CharType>& a) { return a._data[0]; }
template <uint32_t Capacity, typename CharType> inline const CharType& front(const StackString<Capacity, CharType>& a) { return a._data[0]; }
template <uint32_t Capacity, typename CharType> inline CharType& back(StackString<Capacity, CharType>& a) { return a._data[a._size - 1]; }
template <uint32_t Capacity, typename CharType> inline const CharType& back(const StackString<Capacity, CharType>& a) { return a._data[a._size - 1]; }

template <uint32_t Capacity, typename CharType> inline void clear(StackString<Capacity, CharType>& a) { resize(a, 0); }
template <uint32_t Capacity, typename CharType> inline void trim(StackString<Capacity, CharType>& a) { set_capacity(a, a._size + 1); }

template <uint32_t Capacity, typename CharType> void resize(StackString<Capacity, CharType>& a, uint32_t new_size)
{
    mx_assert(new_size + 1 <= Capacity, "New size is larger than the available capacity! [ capacity={}, size={} ]", Capacity, new_size);
    a._size = new_size;
    a._data[a._size] = 0;
}

template <uint32_t Capacity, typename CharType> inline void reserve(StackString<Capacity, CharType>& a, uint32_t new_capacity)
{
    mx_assert(new_capacity <= Capacity, "Requested capacity is larger than the available capacity! [ available={}, requested={} ]", Capacity, new_capacity);
}

template <uint32_t Capacity, typename CharType> void set_capacity(StackString<Capacity, CharType>& a, uint32_t new_capacity)
{
    mx_assert(new_capacity <= Capacity, "Requested capacity is larger than the available capacity! [ available={}, requested={} ]", Capacity, new_capacity);
}

template <uint32_t Capacity, typename CharType> void grow(StackString<Capacity, CharType>& a, uint32_t min_capacity)
{
    mx_assert(min_capacity <= Capacity, "Trying to grow over the available capacity! [ available={}, requested={} ]", Capacity, min_capacity);
}

template <uint32_t Capacity, typename CharType> inline void push_back(StackString<Capacity, CharType>& a, const CharType& item)
{
    grow(a._size + 1);
    a._data[a._size] = item;
    a._data[a._size++] = 0;
}

template <uint32_t Capacity, typename CharType> inline void push_back(StackString<Capacity, CharType>& a, const CharType* character_array)
{
    auto str_len = strlen(character_array);
    if (str_len > 0)
    {
        auto new_size = a._size + str_len;
        grow(a, new_size + 1);

        memcpy(string::end(a), character_array, str_len);
        a._size = new_size;
        a._data[a._size] = 0;
    }
}

template <uint32_t Capacity, typename CharType> inline void pop_back(StackString<Capacity, CharType> & a)
{
    a._data[--a._size] = 0;
}

template <uint32_t Capacity, typename CharType>
void pop_back(StackString<Capacity, CharType>& a, uint32_t num)
{
    if (a._size < num)
        a._size = num;
    a._size -= num;
    a._data[a._size] = 0;
}

}

template <uint32_t Capacity, typename CharType>
inline StackString<Capacity, CharType>::StackString() : _size(0), _data{ } {}

template <uint32_t Capacity, typename CharType>
inline StackString<Capacity, CharType>::StackString(const char* cstring) : _size(0), _data{ }
{
    *this = cstring;
}

template <uint32_t Capacity, typename CharType>
inline StackString<Capacity, CharType>::~StackString()
{ }

template <uint32_t Capacity, typename CharType>
template <uint32_t OtherCapacity>
inline StackString<Capacity, CharType>::StackString(const StackString<OtherCapacity, CharType>& other) : _size(0), _data{ }
{
    static constexpr auto min_capacity = Capacity < OtherCapacity ? Capacity : OtherCapacity;

    const uint32_t n = min_capacity < other._size ? min_capacity : other._size;
    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
}

template <uint32_t Capacity, typename CharType>
template <uint32_t OtherCapacity>
inline StackString<Capacity, CharType>& StackString<Capacity, CharType>::operator=(const StackString<OtherCapacity, CharType>& other)
{
    static constexpr auto min_capacity = Capacity < OtherCapacity ? Capacity : OtherCapacity;
    const uint32_t n = min_capacity <= other._size ? min_capacity - 1 : other._size;
    string::resize(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * (n + 1));
    return *this;
}

template<uint32_t Capacity, typename CharType>
inline StackString<Capacity, CharType>& StackString<Capacity, CharType>::operator=(const String<CharType>& other)
{
    const uint32_t n = min_capacity < other._size ? Capacity : other._size;
    string::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(CharType) * n);
    _size = n - 1;
    return *this;
}

template <uint32_t Capacity, typename CharType>
inline StackString<Capacity, CharType>& StackString<Capacity, CharType>::operator=(const CharType* cstring)
{
    const uint32_t str_len = strlen(cstring);
    const uint32_t n = Capacity < str_len ? Capacity : str_len;
    string::set_capacity(*this, n);
    memcpy(_data, cstring, sizeof(CharType) * n);
    _size = n;
    return *this;
}

template <uint32_t Capacity, typename CharType>
inline CharType& StackString<Capacity, CharType>::operator[](uint32_t i)
{
    return _data[i];
}

template <uint32_t Capacity, typename CharType>
inline const CharType& StackString<Capacity, CharType>::operator[](uint32_t i) const
{
    return _data[i];
}

// range based ADL lookup
template <uint32_t Capacity, typename CharType> inline CharType* begin(StackString<Capacity, CharType>& a)
{
    return string::begin(a);
}

template <uint32_t Capacity, typename CharType> inline const CharType* begin(const StackString<Capacity, CharType>& a)
{
    return string::begin(a);
}

template <uint32_t Capacity, typename CharType> inline CharType* end(StackString<Capacity, CharType>& a)
{
    return string::end(a);
}

template <uint32_t Capacity, typename CharType> inline const CharType* end(const StackString<Capacity, CharType>& a)
{
    return string::end(a);
}

template <uint32_t Capacity, typename CharType> void swap(StackString<Capacity, CharType>& lhs, StackString<Capacity, CharType>& rhs)
{
    std::swap(lhs._allocator, rhs._allocator);
    std::swap(lhs._size, rhs._size);
    std::swap(lhs._capacity, rhs._capacity);
    std::swap(lhs._data, rhs._data);
}

} // namespace pod
