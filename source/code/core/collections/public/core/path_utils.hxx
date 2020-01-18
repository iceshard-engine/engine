#pragma once
#include <core/string.hxx>
#include <core/string_view.hxx>

#if _MSC_VER < 1924
using char8_t = char;
#endif

namespace core::path
{

    namespace constant
    {

        static constexpr char extension_separator = '.';

        static constexpr core::StringView directory_separators = "/\\";

    }

    [[nodiscard]]
    auto extension(core::StringView str) noexcept -> core::StringView;

    [[nodiscard]]
    auto filename(core::StringView str) noexcept -> core::StringView;

    [[nodiscard]]
    auto directory(core::StringView str) noexcept -> core::StringView;

    auto replace_filename(core::String<>& str, core::StringView filename) noexcept -> core::StringView;

} // namespace core::path
