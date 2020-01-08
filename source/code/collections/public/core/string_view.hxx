#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>
#include <fmt/format.h>

namespace core
{

    namespace cstring
    {

        namespace detail
        {

            template<typename CharType>
            constexpr uint32_t length_internal(CharType const* str, uint32_t len_acc) noexcept
            {
                if (*str == 0)
                {
                    return len_acc;
                }
                else
                {
                    return length_internal(str + 1, len_acc + 1);
                }
            }

        } // namespace detail

        template<typename CharType>
        constexpr uint32_t length(CharType const* str) noexcept
        {
            return detail::length_internal(str, 0);
        }

    } // namespace cstring

    namespace string
    {

        //! \brief Size of the string.
        template<typename CharType>
        auto size(const StringView<CharType>& str) noexcept -> uint32_t;

        //! \brief Length of the string.
        template<typename CharType>
        auto length(StringView<CharType> const str) noexcept -> uint32_t;

        //! \brief The current string capacity.
        template<typename CharType>
        auto capacity(const StringView<CharType>& str) noexcept -> uint32_t;

        //! \brief Checks if the given string is empty.
        template<typename CharType>
        bool empty(const StringView<CharType>& str) noexcept;

        //! \copydoc core::string::begin(String<CharType>&)
        template<typename CharType>
        auto begin(const StringView<CharType>& a) noexcept -> const CharType*;

        //! \copydoc core::string::end(String<CharType>&)
        template<typename CharType>
        auto end(const StringView<CharType>& str) noexcept -> const CharType*;

        //! \copydoc core::string::front(String<CharType>&)
        template<typename CharType>
        auto front(const StringView<CharType>& str) noexcept -> const CharType&;

        //! \copydoc core::string::back(String<CharType>&)
        template<typename CharType>
        auto back(const StringView<CharType>& str) noexcept -> const CharType&;

        //! \brief Clears the string view object.
        template<typename CharType>
        void clear(StringView<CharType>& str) noexcept;

        //////////////////////////////////////////////////////////////////////////

        auto find_first_of(StringView<> const str, char character_value) noexcept -> char const*;

        auto find_first_of(StringView<> const str, StringView<> const character_values) noexcept -> char const*;

        auto find_last_of(StringView<> const str, char character_value) noexcept -> char const*;

        auto find_last_of(StringView<> const str, StringView<> const character_value) noexcept -> char const*;

        //////////////////////////////////////////////////////////////////////////

        template<typename CharType>
        bool equals(StringView<CharType> const left, StringView<CharType> right) noexcept;

        template<typename CharType>
        bool equals(StringView<CharType> const left, String<CharType> const& right) noexcept;

        template<uint32_t Capacity, typename CharType>
        bool equals(StringView<CharType> const left, StackString<Capacity, CharType> const& right) noexcept;

        template<typename CharType>
        bool equals(StringView<CharType> const left, std::string_view right) noexcept;

        template<typename CharType>
        bool equals(StringView<CharType> const left, CharType const* right) noexcept;

    } // namespace string

#include "string_view.inl"

} // namespace core

// core::String FTM formatter
//////////////////////////////////////////////////////////////////////////

namespace fmt
{

    template<typename CharType>
    struct formatter<core::StringView<CharType>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const core::StringView<CharType>& str, FormatContext& ctx)
        {
            return fmt::format_to(ctx.begin(), std::string_view{ str._data, str._size });
        }
    };

} // namespace fmt
