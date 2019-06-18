#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>

namespace core
{
namespace string
{

//! \brief Size of the string.
template <typename CharType>
auto size(const String<CharType>& str) noexcept -> uint32_t;

//! \brief Length of the string.
template <typename CharType>
auto length(const String<CharType>& str) noexcept -> uint32_t;

//! \brief The current string capacity.
template<typename CharType>
auto capacity(const String<CharType>& str) noexcept -> uint32_t;

//! \brief Checks if the given string is empty.
template <typename CharType>
bool empty(const String<CharType>& str) noexcept;

//! \brief Returns the string iterator beginning.
template <typename CharType>
auto begin(String<CharType>& a) noexcept -> CharType*;

//! \copydoc begin(String<CharType>&)
template <typename CharType>
auto begin(const String<CharType>& a) noexcept -> const CharType*;

//! \brief Returns the string iterator end.
template <typename CharType>
auto end(String<CharType>& str) noexcept -> CharType*;

//! \copydoc end(String<CharType>&)
template <typename CharType>
auto end(const String<CharType>& str) noexcept -> const CharType*;

//! \brief Returns the first element of the string.
//! \note Cannot be used on an empty string.
template <typename CharType>
auto front(String<CharType>& str) noexcept -> CharType&;

//! \copydoc front(String<CharType>&)
template <typename CharType>
auto front(const String<CharType>& str) noexcept -> const CharType&;

//! \brief Returns the last element of the string.
//! \note Cannot be used on an empty string.
template <typename CharType>
auto back(String<CharType>& str) noexcept -> CharType&;

//! \copydoc back(String<CharType>&)
template <typename CharType>
auto back(const String<CharType>& str) noexcept -> const CharType&;

//! \brief Resizes the string to the requested value.
//! \details The resized string does not initialize the new acquired data, thus
//!     it might contain undefined values.
//!
//! \note This function does not reallocate memory unless necessary.
template <typename CharType>
void resize(String<CharType>& str, uint32_t new_size) noexcept;

//! \brief Clears the string but does not free the memory.
template <typename CharType>
void clear(String<CharType>& str) noexcept;

//! \brief Reallocates the string to the specified capacity.
//! \note If the capacity is set to 0, the string is released totally.
template <typename CharType>
void set_capacity(String<CharType>& str, uint32_t new_capacity) noexcept;

//! \brief Ensures the string has at least the specified capacity.
template <typename CharType>
void reserve(String<CharType>& str, uint32_t new_capacity) noexcept;

//! \brief Grows the string using a geometric progression formula.
//!
//! \details This amortizes the cost of push_back() to O(1).
//! \details If a min_capacity is specified, the string will grow to at least that capacity.
template <typename CharType>
void grow(String<CharType>& str, uint32_t min_capacity = 0) noexcept;

//! \brief Trims the string so that its capacity matches its size.
template <typename CharType>
void trim(String<CharType>& str) noexcept;

//! \brief Pushes the character to the end of the string.
template <typename CharType>
void push_back(String<CharType>& str, const CharType& character) noexcept;

//! \brief Pushes the character array to the end of the string.
template <typename CharType>
void push_back(String<CharType>& str, const CharType* character_array) noexcept;

//! \brief Pops the last character from the string. The string cannot be empty.
template <typename CharType>
void pop_back(String<CharType>& str) noexcept;

//! \brief Pops the last N characters from the string. The string cannot be empty.
template <typename CharType>
void pop_back(String<CharType>& str, uint32_t num) noexcept;

} // namespace string


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


#include "string.inl"


} // namespace core::string
