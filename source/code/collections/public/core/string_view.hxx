#pragma once
#include <core/base.hxx>
#include <core/string_types.hxx>
#include <fmt/format.h>

namespace core
{


    namespace string
    {

        //! \brief Size of the string.
        template <typename CharType>
        auto size(const StringView<CharType>& str) noexcept->uint32_t;

        //! \brief Length of the string.
        template <typename CharType>
        auto length(const StringView<CharType>& str) noexcept->uint32_t;

        //! \brief The current string capacity.
        template<typename CharType>
        auto capacity(const StringView<CharType>& str) noexcept->uint32_t;

        //! \brief Checks if the given string is empty.
        template <typename CharType>
        bool empty(const StringView<CharType>& str) noexcept;

        //! \brief Returns the string iterator beginning.
        template <typename CharType>
        auto begin(StringView<CharType>& a) noexcept->CharType*;

        //! \copydoc core::string::begin(String<CharType>&)
        template <typename CharType>
        auto begin(const StringView<CharType>& a) noexcept -> const CharType*;

        //! \brief Returns the string iterator end.
        template <typename CharType>
        auto end(StringView<CharType>& str) noexcept->CharType*;

        //! \copydoc core::string::end(String<CharType>&)
        template <typename CharType>
        auto end(const StringView<CharType>& str) noexcept -> const CharType*;

        //! \brief Returns the first element of the string.
        //! \note Cannot be used on an empty string.
        template <typename CharType>
        auto front(StringView<CharType>& str) noexcept->CharType&;

        //! \copydoc core::string::front(String<CharType>&)
        template <typename CharType>
        auto front(const StringView<CharType>& str) noexcept -> const CharType&;

        //! \brief Returns the last element of the string.
        //! \note Cannot be used on an empty string.
        template <typename CharType>
        auto back(StringView<CharType>& str) noexcept->CharType&;

        //! \copydoc core::string::back(String<CharType>&)
        template <typename CharType>
        auto back(const StringView<CharType>& str) noexcept -> const CharType&;

        //! \brief Clears the string view object.
        template <typename CharType>
        void clear(StringView<CharType>& str) noexcept;


        //////////////////////////////////////////////////////////////////////////


        template <typename CharType>
        bool equals(const StringView<CharType>& left, const StringView<CharType>& right) noexcept;

        template <typename CharType>
        bool equals(const StringView<CharType>& left, const String<CharType>& right) noexcept;

        template <uint32_t Capacity, typename CharType>
        bool equals(const StringView<CharType>& left, const StackString<Capacity, CharType>& right) noexcept;

        template <typename CharType>
        bool equals(const StringView<CharType>& left, const std::string_view right) noexcept;

        template <typename CharType>
        bool equals(const StringView<CharType>& left, const CharType* right) noexcept;

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
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const core::StringView<CharType>& str, FormatContext& ctx)
        {
            return fmt::format_to(ctx.begin(), std::string_view{ str._data, str._size });
        }
    };

} // namespace fmt
