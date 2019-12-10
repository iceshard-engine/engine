#pragma once
#include <core/allocator.hxx>
#include <string_view>

namespace core
{

    //! \brief A heap allocated string object.
    template<typename CharType = char>
    struct String;

    //! \brief A stack allocated string object.
    //! \details The string capacity is a constant value.
    template<uint32_t Size = 16, typename CharType = char>
    struct StackString;

    //! \brief A view into a string.
    template<typename CharType = char>
    struct StringView;

    //! \brief A heap allocated string object.
    template<typename CharType>
    struct String
    {
        //! \brief Creates a new String object with the given allocator.
        String(core::allocator& a) noexcept;

        //! \brief Creates a new String object with the given allocator and value to copy.
        String(core::allocator& a, const CharType* other) noexcept;

        //! \brief Moves a given String object.
        String(String&& other) noexcept;

        //! \brief Copies a given String object using the same allocator.
        String(const String& other) noexcept;

        //! \brief Destroys the string object.
        ~String() noexcept;

        //! \brief Swaps the string value with the new one.
        auto operator=(String&& other) noexcept -> String&;

        //! \brief Replaces the string value with the new one.
        auto operator=(const String& other) noexcept -> String&;

        //! \brief Replaces the string value with the new one.
        template<uint32_t Capacity>
        auto operator=(const StackString<Capacity, CharType>& other) noexcept -> String&;

        //! \brief Replaces the string value with the new one.
        auto operator=(const CharType* other) noexcept -> String&;

        //! \brief Returns the character at the given position.
        auto operator[](uint32_t i) noexcept -> CharType&;

        //! \brief Returns the character at the given position.
        auto operator[](uint32_t i) const noexcept -> const CharType&;

        //! \brief The allocator used to manage memory.
        core::allocator* _allocator;

        //! \brief The actual size.
        uint32_t _size{ 0 };

        //! \brief The actual capacity.
        uint32_t _capacity{ 0 };

        //! \brief The string data.
        CharType* _data{ nullptr };
    };

    //! \brief A stack allocated string object.
    //! \details The string capacity is a constant value.
    template<uint32_t Capacity, typename CharType>
    struct StackString
    {
        //! \brief Creates a new StackString object.
        StackString() noexcept = default;

        //! \brief Creates a new StackString object with the given value.
        StackString(const CharType* cstring) noexcept;

        //! \brief Creates a new StackString object from another StackString.
        //! \details If the input StackString is larger, it will only copy the maximum
        //!     amount of characters the rest will be discarded.
        template<uint32_t OtherCapacity>
        StackString(const StackString<OtherCapacity, CharType>& other) noexcept;

        //! \brief Destroys the stack string
        ~StackString() noexcept = default;

        //! \brief Replaces the string value with the new one.
        template<uint32_t Capacity>
        auto operator=(const StackString<Capacity, CharType>& other) noexcept -> StackString&;

        //! \brief Replaces the string value with the new one.
        //! \details If the input String is larger, it will only copy the maximum
        //!     amount of characters the rest will be discarded.
        auto operator=(const String<CharType>& other) noexcept -> StackString&;

        //! \brief Replaces the string value with the new one.
        auto operator=(const CharType* other) noexcept -> StackString&;

        //! \brief Returns the character at the given position.
        auto operator[](uint32_t i) noexcept -> CharType&;

        //! \brief Returns the character at the given position.
        auto operator[](uint32_t i) const noexcept -> const CharType&;

        //! \brief The actual size.
        uint32_t _size{ 0 };

        //! \brief The string data buffer.
        CharType _data[Capacity]{ '\0' };
    };

    //! \brief A view into a string.
    template<typename CharType>
    struct StringView
    {
        //! \brief Creates a new StackString object.
        constexpr StringView() noexcept = default;

        constexpr StringView(CharType const* cstring) noexcept;

        constexpr StringView(CharType const* beg, CharType const* end) noexcept;

        template<size_t Size>
        constexpr StringView(CharType const (&cstring)[Size]) noexcept;

        //! \brief Creates a new StackString object with the given value.
        StringView(std::string const& str_view) noexcept;

        //! \brief Creates a new StackString object with the given value.
        constexpr StringView(std::string_view str_view) noexcept;

        //! \brief Creates a new StringView from a String.
        StringView(String<CharType> const& other) noexcept;

        //! \brief Creates a new StringView from a StackString.
        template<uint32_t Capacity>
        StringView(StackString<Capacity, CharType> const& other) noexcept;

        //! \brief Destroys the stack string
        ~StringView() noexcept = default;

        //! \brief Replaces the string value with the new one.
        auto operator=(CharType const* other) noexcept -> StringView&;

        //! \brief Replaces the string value with the new one.
        auto operator=(std::string_view other) noexcept -> StringView&;

        //! \brief Replaces the string value with the new one.
        auto operator=(StringView const& other) noexcept -> StringView& = default;

        //! \brief Replaces the string value with the new one.
        template<uint32_t Capacity>
        auto operator=(StackString<Capacity, CharType> const& other) noexcept -> StringView&;

        //! \brief Replaces the string value with the new one.
        //! \details If the input String is larger, it will only copy the maximum
        //!     amount of characters the rest will be discarded.
        auto operator=(String<CharType> const& other) noexcept -> StringView&;

        //! \brief Returns the character at the given position.
        constexpr auto operator[](uint32_t i) const noexcept -> const CharType&;

        //! \brief The actual size.
        uint32_t _size{ 0 };

        //! \brief The string data buffer.
        const CharType* _data;
    };

} // namespace core
