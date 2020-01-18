#include "string_view.hxx"

//! \brief Size of the string.
inline auto core::string::size(core::StringView str) noexcept -> uint32_t
{
    return static_cast<uint32_t>(str.size());
}

//! \brief Length of the string.
inline auto core::string::length(core::StringView str) noexcept -> uint32_t
{
    return static_cast<uint32_t>(str.length());
}

inline auto core::string::data(core::StringView str) noexcept -> core::StringView::value_type const*
{
    return str.data();
}

//! \brief The current string capacity.
inline auto core::string::capacity(core::StringView str) noexcept -> uint32_t
{
    return core::string::size(str);
}

//! \brief Checks if the given string is empty.
inline bool core::string::empty(core::StringView str) noexcept
{
    return str.empty();
}

//! \copydoc core::string::begin(const String<CharType>&)
inline auto core::string::begin(core::StringView str) noexcept -> core::StringView::const_iterator
{
    return str.cbegin();
}

//! \copydoc core::string::end(const String<CharType>&)
inline auto core::string::end(core::StringView str) noexcept -> core::StringView::const_iterator
{
    return str.cend();
}

//! \copydoc core::string::begin(const String<CharType>&)
inline auto core::string::rbegin(core::StringView str) noexcept -> core::StringView::const_reverse_iterator
{
    return str.crbegin();
}

//! \copydoc core::string::end(const String<CharType>&)
inline auto core::string::rend(core::StringView str) noexcept -> core::StringView::const_reverse_iterator
{
    return str.crend();
}

//! \copydoc core::string::front(String<CharType>&)
inline auto core::string::front(core::StringView str) noexcept -> core::StringView::value_type
{
    return str.front();
}

//! \copydoc core::string::back(String<CharType>&)
inline auto core::string::back(core::StringView str) noexcept -> core::StringView::value_type
{
    return str.back();
}

//! \brief Clears the string view object.
inline void core::string::clear(core::StringView& str) noexcept
{
    str = core::StringView{};
}

inline auto core::string::substr(StringView str, uint32_t pos, uint32_t len) noexcept -> core::StringView
{
    return str.substr(pos, len == core::string::npos ? std::string_view::npos : len);
}

//////////////////////////////////////////////////////////////////////////

inline auto core::string::find_first_of(StringView str, char character_value) noexcept -> uint32_t
{
    auto const result = str.find_first_of(character_value);
    return result == std::string::npos ? core::string::npos : static_cast<uint32_t>(result);
}

inline auto core::string::find_first_of(StringView str, StringView character_values) noexcept -> uint32_t
{
    auto const result = str.find_first_of(character_values);
    return result == std::string::npos ? core::string::npos : static_cast<uint32_t>(result);
}

inline auto core::string::find_last_of(StringView str, char character_value) noexcept -> uint32_t
{
    auto const result = str.find_last_of(character_value);
    return result == std::string::npos ? core::string::npos : static_cast<uint32_t>(result);
}

inline auto core::string::find_last_of(StringView str, StringView character_values) noexcept -> uint32_t
{
    auto const result = str.find_last_of(character_values);
    return result == std::string::npos ? core::string::npos : static_cast<uint32_t>(result);
}

//////////////////////////////////////////////////////////////////////////

inline bool core::string::equals(core::StringView left, core::StringView right) noexcept
{
    return left == right;
}
