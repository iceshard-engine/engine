#pragma once
#include <ice/mem_allocator.hxx>
#include <string_view>

namespace ice
{

    template<typename CharType>
    using BasicString = std::basic_string_view<CharType>;

    using String = BasicString<char>;

    using Utf8String = BasicString<char8_t>;
    using AsciiString = BasicString<char>;

#if ISP_WINDOWS
    using WString = BasicString<wchar_t>;
#endif

    template<typename CharType = char>
    struct HeapString
    {
        using value_type = CharType;
        using iterator = value_type*;
        using reverse_iterator = std::reverse_iterator<value_type*>;
        using const_iterator = value_type const*;
        using const_reverse_iterator = std::reverse_iterator<value_type const*>;

        explicit HeapString(ice::Allocator& allocator) noexcept;
        HeapString(ice::Allocator& allocator, ice::BasicString<CharType> string) noexcept;
        ~HeapString() noexcept;

        HeapString(HeapString&& other) noexcept;
        HeapString(HeapString const& other) noexcept;

        auto operator=(HeapString&& other) noexcept -> HeapString&;
        auto operator=(HeapString const& other) noexcept -> HeapString&;

        auto operator=(ice::BasicString<CharType> str) noexcept -> HeapString&;

        auto operator[](uint32_t index) noexcept -> CharType&;
        auto operator[](uint32_t index) const noexcept -> CharType const&;

        operator ice::BasicString<CharType>() const noexcept;

        ice::Allocator* _allocator;

        uint32_t _size{ 0 };
        uint32_t _capacity{ 0 };
        value_type* _data{ nullptr };
    };

    template<uint32_t Size = 16, typename CharType = char>
    struct StackString
    {
        using value_type = CharType;
        using iterator = value_type*;
        using reverse_iterator = std::reverse_iterator<value_type*>;
        using const_iterator = value_type const*;
        using const_reverse_iterator = std::reverse_iterator<value_type const*>;

        static constexpr uint32_t Constant_Capacity = Size;

        StackString() noexcept = default;
        template<uint32_t ArraySize>
        StackString(CharType (&char_array)[ArraySize]) noexcept;
        StackString(ice::BasicString<CharType> str) noexcept;
        ~StackString() noexcept = default;

        template<uint32_t OtherSize>
        StackString(StackString<OtherSize, CharType> const& other) noexcept;

        template<uint32_t OtherSize>
        auto operator=(StackString<OtherSize, CharType> const& other) noexcept -> StackString&;

        auto operator=(ice::BasicString<CharType> str) noexcept -> StackString&;

        auto operator[](uint32_t i) noexcept -> CharType&;
        auto operator[](uint32_t i) const noexcept -> CharType const&;

        operator ice::BasicString<CharType>() const noexcept;

        uint32_t _size{ 0 };
        CharType _data[Constant_Capacity]{ '\0' };
    };

    static constexpr uint32_t string_npos = std::numeric_limits<uint32_t>::max();

    static constexpr auto operator""_str(char8_t const* str, std::size_t size) noexcept
    {
        return ice::Utf8String{ str, size };
    }

    template<typename T>
    bool from_chars(ice::String str, ice::String& out_remaining, T& out_value) noexcept
    {
#if ISP_WINDOWS
        std::from_chars_result const result = std::from_chars(str.data(), str.data() + str.size(), out_value);
        out_remaining = { result.ptr, str.data() + str.size() };
        return result.ec == std::errc{};
#else
        return false;
#endif
    }

    template<typename T>
    bool from_chars(ice::Utf8String str, ice::Utf8String& out_remaining, T& out_value) noexcept
    {
#if ISP_WINDOWS
        char const* data_ptr = reinterpret_cast<char const*>(str.data());

        std::from_chars_result const result = std::from_chars(data_ptr, data_ptr + str.size(), out_value);
        out_remaining = {
            reinterpret_cast<ice::utf8 const*>(result.ptr),
            reinterpret_cast<ice::utf8 const*>(data_ptr + str.size())
        };
        return result.ec == std::errc{};
#else
        return false;
#endif
    }

} // namespace ice
