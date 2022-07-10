#pragma once
#include <ice/string.hxx>

namespace ice
{

    struct Font;

    auto font_text_bounds(
        ice::Font const& font,
        ice::Utf8String text,
        ice::u32& out_glyph_count
    ) noexcept -> ice::vec2f;

    auto font_text_bounds(
        ice::Font const& font,
        ice::Utf8String text
    ) noexcept -> ice::vec2f;

    auto text_get_codepoint(
        ice::c8utf const* data,
        ice::u32& out_bytes
    ) noexcept -> ice::u32;

} // namespace ice
