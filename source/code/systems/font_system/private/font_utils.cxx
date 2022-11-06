/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/font_utils.hxx>
#include <ice/math/common.hxx>
#include <ice/span.hxx>
#include <ice/font.hxx>
#include <ice/log.hxx>

namespace ice
{

    namespace detail
    {

        constexpr auto consume_next_character(
            char const* it,
            ice::u32& out_bytes
        ) noexcept -> ice::u32
        {
            constexpr ice::u32 utf8_1byte_mask = 0b1000'0000;
            constexpr ice::u32 utf8_2byte_mask = 0b1110'0000;
            constexpr ice::u32 utf8_3byte_mask = 0b1111'0000;
            constexpr ice::u32 utf8_4byte_mask = 0b1111'1000;
            constexpr ice::u32 utf8_vbyte_mask = 0b0011'1111;
            constexpr ice::u32 utf8_1byte_count = 0;
            constexpr ice::u32 utf8_2byte_count = 0b1100'0000;
            constexpr ice::u32 utf8_3byte_count = 0b1110'0000;
            constexpr ice::u32 utf8_4byte_count = 0b1111'0000;

            if ((*it & utf8_1byte_mask) == utf8_1byte_count)
            {
                out_bytes = 1;
                return ice::u8(it[0]);
            }
            if ((*it & utf8_2byte_mask) == utf8_2byte_count)
            {
                out_bytes = 2;
                ice::u32 codepoint = ice::u8(it[0] & ~utf8_2byte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[1] & utf8_vbyte_mask);
                return codepoint;
            }
            if ((*it & utf8_3byte_mask) == utf8_3byte_count)
            {
                out_bytes = 3;
                ice::u32 codepoint = ice::u8(it[0] & ~utf8_3byte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[1] & utf8_vbyte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[2] & utf8_vbyte_mask);
                return codepoint;
            }
            if ((*it & utf8_4byte_mask) == utf8_4byte_count)
            {
                out_bytes = 4;
                ice::u32 codepoint = ice::u8(it[0] & ~utf8_4byte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[1] & utf8_vbyte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[2] & utf8_vbyte_mask);
                codepoint <<= 6;
                codepoint |= ice::u8(it[3] & utf8_vbyte_mask);
                return codepoint;
            }
            return 0;
        }

    } // namespace detail

    auto font_text_bounds(
        ice::Font const& font,
        ice::String text,
        ice::u32& out_glyph_count
    ) noexcept -> ice::vec2f
    {
        char const* text_it = ice::string::begin(text);
        char const* const text_end = ice::string::end(text);

        ice::f32 last_advance_offset = 0.f;

        ice::u32 total_glyphs = 0;
        ice::vec2f result{ 0.f };
        while (text_it != text_end)
        {
            ice::u32 consumed_bytes;
            ice::u32 const codepoint = detail::consume_next_character(text_it, consumed_bytes);

            for (ice::Glyph const& glyph : font.glyphs)
            {
                if (glyph.codepoint == codepoint)
                {
                    result.x += glyph.advance;
                    result.y = ice::max(-glyph.size.y, result.y);
                    last_advance_offset = glyph.advance - glyph.size.x;

                    out_glyph_count += 1;
                }
            }

            text_it += consumed_bytes;
            total_glyphs += 1;
        }

        result.x -= last_advance_offset;

        ICE_LOG_IF(
            total_glyphs != out_glyph_count,
            ice::LogSeverity::Warning, ice::LogTag::Engine,
            "The given font couldn't resolve all glyphs in the given text '{}'. Missing {} glyphs.",
            text,
            total_glyphs
        );

        return result;
    }

    auto font_text_bounds(
        ice::Font const& font,
        ice::String text
    ) noexcept -> ice::vec2f
    {
        [[maybe_unused]]
        ice::u32 unused = 0;
        return font_text_bounds(font, text, unused);
    }

    auto text_get_codepoint(char const* data, ice::u32& out_bytes) noexcept -> ice::u32
    {
        return detail::consume_next_character(data, out_bytes);
    }

} // namespace ice
