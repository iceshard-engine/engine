#pragma once
#include <ice/allocator.hxx>
#include <string_view>

namespace ice
{

    template<typename CharType>
    using BasicString = std::basic_string_view<CharType>;

    using String = BasicString<char>;

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
        StackString(CharType(&char_array)[ArraySize]) noexcept;
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

} // namespace ice
