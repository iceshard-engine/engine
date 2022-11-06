/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    struct Font;

    auto font_text_bounds(
        ice::Font const& font,
        ice::String text,
        ice::u32& out_glyph_count
    ) noexcept -> ice::vec2f;

    auto font_text_bounds(
        ice::Font const& font,
        ice::String text
    ) noexcept -> ice::vec2f;

    auto text_get_codepoint(
        char const* data,
        ice::u32& out_bytes
    ) noexcept -> ice::u32;

} // namespace ice
