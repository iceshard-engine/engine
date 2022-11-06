/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/math.hxx>
#include <ice/asset_type.hxx>

namespace ice
{

    static constexpr ice::AssetType AssetType_Font = ice::make_asset_type("ice/font");
    static constexpr ice::AssetType AssetType_FontAtlas = ice::make_asset_type("ice/font_atlas");

    struct Glyph
    {
        ice::u32 codepoint;
        ice::vec2f offset;
        ice::vec2f size;
        ice::f32 advance;
        ice::f32 atlas_x;
        ice::f32 atlas_y;
        ice::f32 atlas_w;
        ice::f32 atlas_h;
    };

    enum class GlyphRangeType : ice::u32
    {
        Explicit,
        Range
    };

    struct GlyphRange
    {
        ice::GlyphRangeType type;
        ice::u32 glyph_atlas;
        ice::u32 glyph_index;
        ice::u32 glyph_count;
    };

    struct FontGlyphRange
    {
        ice::GlyphRangeType type;
        ice::u32 glyph_atlas;
        ice::u32 glyph_index;
        ice::u32 glyph_count;
    };

    struct FontAtlas
    {
        ice::vec2u image_size;
        ice::u32 image_data_offset;
        ice::u32 image_data_size;
    };

    struct Font
    {
        ice::Span<ice::FontAtlas const> atlases;
        ice::Span<ice::GlyphRange const> ranges;
        ice::Span<ice::Glyph const> glyphs;
        void const* data_ptr;
    };

} // namespace ice::font
