#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>
#include <fmt/format.h>

namespace core
{

    namespace string
    {

        //! \brief Size of the string.
        template<typename CharType>
        auto size(const String<CharType>& str) noexcept -> uint32_t;

        //! \brief Length of the string.
        template<typename CharType>
        auto length(const String<CharType>& str) noexcept -> uint32_t;

        //! \brief The current string capacity.
        template<typename CharType>
        auto capacity(const String<CharType>& str) noexcept -> uint32_t;

        //! \brief Checks if the given string is empty.
        template<typename CharType>
        bool empty(const String<CharType>& str) noexcept;

        //! \brief Returns the string iterator beginning.
        template<typename CharType>
        auto begin(String<CharType>& a) noexcept -> CharType*;

        //! \copydoc core::string::begin(String<CharType>&)
        template<typename CharType>
        auto begin(const String<CharType>& a) noexcept -> const CharType*;

        //! \brief Returns the string iterator end.
        template<typename CharType>
        auto end(String<CharType>& str) noexcept -> CharType*;

        //! \copydoc core::string::end(String<CharType>&)
        template<typename CharType>
        auto end(const String<CharType>& str) noexcept -> const CharType*;

        //! \brief Returns the first element of the string.
        //! \note Cannot be used on an empty string.
        template<typename CharType>
        auto front(String<CharType>& str) noexcept -> CharType&;

        //! \copydoc core::string::front(String<CharType>&)
        template<typename CharType>
        auto front(const String<CharType>& str) noexcept -> const CharType&;

        //! \brief Returns the last element of the string.
        //! \note Cannot be used on an empty string.
        template<typename CharType>
        auto back(String<CharType>& str) noexcept -> CharType&;

        //! \copydoc core::string::back(String<CharType>&)
        template<typename CharType>
        auto back(const String<CharType>& str) noexcept -> const CharType&;

        //! \brief Resizes the string to the requested value.
        //! \details The resized string does not initialize the new acquired data, thus
        //!     it might contain undefined values.
        //!
        //! \note This function does not reallocate memory unless necessary.
        template<typename CharType>
        void resize(String<CharType>& str, uint32_t new_size) noexcept;

        //! \brief Clears the string but does not free the memory.
        template<typename CharType>
        void clear(String<CharType>& str) noexcept;

        //! \brief Reallocates the string to the specified capacity.
        //! \note If the capacity is set to 0, the string is released totally.
        template<typename CharType>
        void set_capacity(String<CharType>& str, uint32_t new_capacity) noexcept;

        //! \brief Ensures the string has at least the specified capacity.
        template<typename CharType>
        void reserve(String<CharType>& str, uint32_t new_capacity) noexcept;

        //! \brief Grows the string using a geometric progression formula.
        //!
        //! \details This amortizes the cost of push_back() to O(1).
        //! \details If a min_capacity is specified, the string will grow to at least that capacity.
        template<typename CharType>
        void grow(String<CharType>& str, uint32_t min_capacity = 0) noexcept;

        //! \brief Trims the string so that its capacity matches its size.
        template<typename CharType>
        void trim(String<CharType>& str) noexcept;

        //! \brief Pushes the character to the end of the string.
        template<typename CharType>
        void push_back(String<CharType>& str, CharType ch) noexcept;

        //! \brief Pushes the character array to the end of the string.
        template <typename CharType>
        void push_back(String<CharType>& str, core::StringView cstr) noexcept;

        //! \brief Pushes the String at the end of the string.
        template<typename CharType>
        void push_back(String<CharType>& str, const String<CharType>& other) noexcept;

        //! \brief Pops the last character from the string. The string cannot be empty.
        template<typename CharType>
        void pop_back(String<CharType>& str) noexcept;

        //! \brief Pops the last N characters from the string. The string cannot be empty.
        template<typename CharType>
        void pop_back(String<CharType>& str, uint32_t num) noexcept;

        template<typename CharType>
        bool equals(const String<CharType>& left, const String<CharType>& right) noexcept;

        template<typename CharType>
        bool equals(const String<CharType>& left, const std::string_view right) noexcept;

        template<typename CharType>
        bool equals(const String<CharType>& left, const CharType* right) noexcept;

    } // namespace string

    //! \copydoc core::string::begin(String<CharType>&)
    template<typename CharType>
    auto begin(String<CharType>& a) noexcept -> CharType*;

    //! \copydoc core::string::begin(String<CharType>&)
    template<typename CharType>
    auto begin(const String<CharType>& a) noexcept -> const CharType*;

    //! \copydoc core::string::end(String<CharType>&)
    template<typename CharType>
    auto end(String<CharType>& a) noexcept -> CharType*;

    //! \copydoc core::string::end(String<CharType>&)
    template<typename CharType>
    auto end(const String<CharType>& a) noexcept -> const CharType*;

    template<typename CharType>
    void swap(String<CharType>& lhs, String<CharType>& rhs) noexcept;

    template<typename CharType>
    auto operator+=(String<CharType>& self, CharType other) noexcept -> String<CharType>&;

    template<typename CharType>
    auto operator+=(String<CharType>& self, const CharType* other) noexcept -> String<CharType>&;

    template<typename CharType>
    auto operator+=(String<CharType>& self, const String<CharType>& other) noexcept -> String<CharType>&;

    template<uint32_t Capacity, typename CharType>
    auto operator+=(String<CharType>& self, const StackString<Capacity, CharType>& other) noexcept -> String<CharType>&;

} // namespace core

#include "string.inl"

namespace fmt
{

    template<typename CharType>
    struct formatter<core::String<CharType>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const core::String<CharType>& str, FormatContext& ctx)
        {
            return fmt::format_to(ctx.begin(), std::string_view{ str._data, str._size });
        }
    };

} // namespace fmt
