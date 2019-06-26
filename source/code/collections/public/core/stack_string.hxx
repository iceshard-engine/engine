#pragma once
#include <core/base.hxx>
#include <core/cexpr/util.hxx>
#include <core/string_types.hxx>
#include <core/debug/assert.hxx>
#include <fmt/format.h>

namespace core
{


// core::StackString deduction guides
//////////////////////////////////////////////////////////////////////////


template<uint32_t Size, typename CharType>
StackString(const CharType(&)[Size]) -> StackString<core::cexpr::power_of_two_roundup(Size, 16u), CharType>;


// core::StackString functions
//////////////////////////////////////////////////////////////////////////


namespace string
{

//! \brief Size of the string.
template<uint32_t Capacity, typename CharType>
auto size(const StackString<Capacity, CharType>& str) noexcept -> uint32_t;

//! \brief Length of the string.
template<uint32_t Capacity, typename CharType>
auto length(const StackString<Capacity, CharType>& str) noexcept -> uint32_t;

//! \brief The current string capacity.
template<uint32_t Capacity, typename CharType>
auto capacity(const StackString<Capacity, CharType>& str) noexcept -> uint32_t;

//! \brief Checks if the given string is empty.
template<uint32_t Capacity, typename CharType>
bool empty(const StackString<Capacity, CharType>& str) noexcept;

//! \brief Returns the string iterator beginning.
template<uint32_t Capacity, typename CharType>
auto begin(StackString<Capacity, CharType>& a) noexcept -> CharType*;

//! \copydoc core::string::begin(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto begin(const StackString<Capacity, CharType>& a) noexcept -> const CharType*;

//! \brief Returns the string iterator end.
template<uint32_t Capacity, typename CharType>
auto end(StackString<Capacity, CharType>& str) noexcept -> CharType*;

//! \copydoc core::string::end(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto end(const StackString<Capacity, CharType>& str) noexcept -> const CharType*;

//! \brief Returns the first element of the string.
//! \note Cannot be used on an empty string.
template<uint32_t Capacity, typename CharType>
auto front(StackString<Capacity, CharType>& str) noexcept -> CharType&;

//! \copydoc core::string::front(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto front(const StackString<Capacity, CharType>& str) noexcept -> const CharType&;

//! \brief Returns the last element of the string.
//! \note Cannot be used on an empty string.
template<uint32_t Capacity, typename CharType>
auto back(StackString<Capacity, CharType>& str) noexcept -> CharType&;

//! \copydoc core::string::back(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto back(const StackString<Capacity, CharType>& str) noexcept -> const CharType&;

//! \brief Resizes the string to the requested value.
//! \details The resized string does not initialize the new acquired data, thus
//!     it might contain undefined values.
//!
//! \note This function does not reallocate memory unless necessary.
template<uint32_t Capacity, typename CharType>
void resize(StackString<Capacity, CharType>& str, uint32_t new_size) noexcept;

//! \brief Clears the string but does not free the memory.
template<uint32_t Capacity, typename CharType>
void clear(StackString<Capacity, CharType>& str) noexcept;

//! \brief Reallocates the string to the specified capacity.
//! \note If the capacity is set to 0, the string is released totally.
template<uint32_t Capacity, typename CharType>
void set_capacity(StackString<Capacity, CharType>& str, uint32_t new_capacity) noexcept;

//! \brief Ensures the string has at least the specified capacity.
template<uint32_t Capacity, typename CharType>
void reserve(StackString<Capacity, CharType>& str, uint32_t new_capacity) noexcept;

//! \brief Grows the string using a geometric progression formula.
//!
//! \details This amortizes the cost of push_back() to O(1).
//! \details If a min_capacity is specified, the string will grow to at least that capacity.
template<uint32_t Capacity, typename CharType>
void grow(StackString<Capacity, CharType>& str, uint32_t min_capacity = 0) noexcept;

//! \brief Trims the string so that its capacity matches its size.
template<uint32_t Capacity, typename CharType>
void trim(StackString<Capacity, CharType>& str) noexcept;

//! \brief Pushes the character to the end of the string.
template<uint32_t Capacity, typename CharType>
void push_back(StackString<Capacity, CharType>& str, CharType character) noexcept;

//! \brief Pushes the character array to the end of the string.
template<uint32_t Capacity, typename CharType>
void push_back(StackString<Capacity, CharType>& str, const CharType* cstr) noexcept;

//! \brief Pushes the StackString value at the end.
template<uint32_t Capacity, typename CharType>
void push_back(StackString<Capacity, CharType>& str, const StackString<Capacity, CharType>& other) noexcept;

//! \brief Pops the last character from the string. The string cannot be empty.
template<uint32_t Capacity, typename CharType>
void pop_back(StackString<Capacity, CharType>& str) noexcept;

//! \brief Pops the last N characters from the string. The string cannot be empty.
template<uint32_t Capacity, typename CharType>
void pop_back(StackString<Capacity, CharType>& str, uint32_t num) noexcept;

} // namespace string


// core::StackString miscelaneous
//////////////////////////////////////////////////////////////////////////


//! \copydoc core::string::begin(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto begin(StackString<Capacity, CharType>& a) noexcept->CharType*;

//! \copydoc core::string::begin(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto begin(const StackString<Capacity, CharType>& a) noexcept -> const CharType*;

//! \copydoc core::string::end(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto end(StackString<Capacity, CharType>& a) noexcept->CharType*;

//! \copydoc core::string::end(StackString<Capacity, CharType>&)
template<uint32_t Capacity, typename CharType>
auto end(const StackString<Capacity, CharType>& a) noexcept -> const CharType*;

template<uint32_t Capacity, typename CharType>
void swap(StackString<Capacity, CharType>& lhs, StackString<Capacity, CharType>& rhs) noexcept;


// core::StackString operators
//////////////////////////////////////////////////////////////////////////


template<uint32_t Capacity, typename CharType>
auto operator+=(StackString<Capacity, CharType>& self, CharType other) noexcept -> StackString<Capacity, CharType>&;

template<uint32_t Capacity, typename CharType>
auto operator+=(StackString<Capacity, CharType>& self, const CharType* other) noexcept -> StackString<Capacity, CharType>&;

template<uint32_t Capacity, typename CharType>
auto operator+=(StackString<Capacity, CharType>& self, const String<CharType>& other) noexcept -> StackString<Capacity, CharType>&;

template<uint32_t Capacity, typename CharType>
auto operator+=(StackString<Capacity, CharType>& self, const StackString<Capacity, CharType>& other) noexcept -> StackString<Capacity, CharType>&;


// core::StackString implementation
//////////////////////////////////////////////////////////////////////////


#include "stack_string.inl"


} // namespace core


// core::StackString FTM formatter
//////////////////////////////////////////////////////////////////////////


namespace fmt
{

template<uint32_t Capacity, typename CharType>
struct formatter<core::StackString<Capacity, CharType>>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const core::StackString<Capacity, CharType>& str, FormatContext& ctx)
    {
        return fmt::format_to(ctx.begin(), std::string_view{ str._data, str._size });
    }
};

} // namespace fmt
