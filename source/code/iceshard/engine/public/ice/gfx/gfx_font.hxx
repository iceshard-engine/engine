#pragma once
#include <ice/asset_type.hxx>
#include <ice/render/render_image.hxx>
#include <ice/pod/hash.hxx>

namespace ice::gfx
{

    static constexpr ice::AssetType AssetType_Font = ice::make_asset_type(u8"ice/gfx/font");

    struct GfxGlyph
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

    enum class GfxGlyphRangeType : ice::u32
    {
        Explicit,
        Range
    };

    struct GfxGlyphRange
    {
        ice::gfx::GfxGlyphRangeType type;
        ice::u32 glyph_atlas;
        ice::u32 glyph_index;
        ice::u32 glyph_count;
    };

    struct GfxFontAtlas
    {
        ice::vec2u image_size;
        ice::u32 image_data_offset;
        ice::u32 image_data_size;
    };

    struct GfxFont
    {
        ice::Span<ice::gfx::GfxFontAtlas const> atlases;
        ice::Span<ice::gfx::GfxGlyphRange const> ranges;
        ice::Span<ice::gfx::GfxGlyph const> glyphs;
        void const* data_ptr;
    };

} // namespace ice::gfx
