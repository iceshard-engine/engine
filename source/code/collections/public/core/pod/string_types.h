#pragma once
#include <core/allocator.hxx>

namespace pod
{

template<typename CharType = char>
struct String;

template<uint32_t Size = 16, typename CharType = char>
struct StackString;

/// A simple string value.
template<typename CharType>
struct String
{
    String(core::allocator& a);
    String(const String& other);
    String& operator=(const String& other);
    ~String();

    CharType& operator[](uint32_t i);
    const CharType& operator[](uint32_t i) const;

    template<uint32_t OtherCapacity>
    String& operator=(const StackString<OtherCapacity, CharType>& other);
    String& operator=(const CharType* other);

    core::allocator* _allocator;
    uint32_t _size;
    uint32_t _capacity;
    CharType* _data;
};

/// A stack allocated string value.
template<uint32_t Capacity, typename CharType>
struct StackString
{
    StackString();
    StackString(const char* cstring);
    template<uint32_t OtherCapacity>
    StackString(const StackString<OtherCapacity, CharType>& other);
    template<uint32_t OtherCapacity>
    StackString& operator=(const StackString<OtherCapacity, CharType>&);
    ~StackString();

    CharType& operator[](uint32_t i);
    const CharType& operator[](uint32_t i) const;

    StackString& operator=(const String<CharType>& other);
    StackString& operator=(const CharType* cstring);

    uint32_t _size;
    CharType _data[Capacity];
};

} // namespace pod
