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
        auto size(core::StringView str) noexcept -> uint32_t;

        //! \brief Length of the string.
        auto length(core::StringView str) noexcept -> uint32_t;

        //! \brief The pointer to the data.
        auto data(core::StringView str) noexcept -> core::StringView::value_type const*;

        //! \brief The current string capacity.
        auto capacity(core::StringView str) noexcept -> uint32_t;

        //! \brief Checks if the given string is empty.
        bool empty(core::StringView str) noexcept;

        //! \copydoc core::string::begin(String<CharType>&)
        auto begin(core::StringView a) noexcept -> core::StringView::const_iterator;

        //! \copydoc core::string::end(String<CharType>&)
        auto end(core::StringView str) noexcept -> core::StringView::const_iterator;

        //! \copydoc core::string::begin(String<CharType>&)
        auto rbegin(core::StringView a) noexcept -> core::StringView::const_reverse_iterator;

        //! \copydoc core::string::end(String<CharType>&)
        auto rend(core::StringView str) noexcept -> core::StringView::const_reverse_iterator;

        //! \copydoc core::string::front(String<CharType>&)
        auto front(core::StringView str) noexcept -> core::StringView::value_type;

        //! \copydoc core::string::back(String<CharType>&)
        auto back(core::StringView str) noexcept -> core::StringView::value_type;

        //! \brief Clears the string view object.
        void clear(core::StringView& str) noexcept;

        //! \brief Returns a substring from the given Position up to N characters.
        auto substr(core::StringView str, uint32_t pos, uint32_t len = core::string::npos) noexcept -> core::StringView;

        //////////////////////////////////////////////////////////////////////////

        auto find_first_of(core::StringView str, char character_value) noexcept -> uint32_t;

        auto find_first_of(core::StringView str, core::StringView character_values) noexcept ->uint32_t;

        auto find_last_of(core::StringView str, char character_value) noexcept -> uint32_t;

        auto find_last_of(core::StringView str, core::StringView character_value) noexcept ->uint32_t;

        //////////////////////////////////////////////////////////////////////////

        bool equals(core::StringView left, core::StringView right) noexcept;

    } // namespace string

} // namespace core

#include "string_view.inl"
