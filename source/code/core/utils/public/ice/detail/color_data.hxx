/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/detail/color_details.hxx>

namespace ice::detail
{

    template<ColorFormat Format>
    struct ColorData;

    template<>
    struct ColorData<ColorFormat::LinearRGBu8>
    {
        ice::u8 red;
        ice::u8 green;
        ice::u8 blue;
        ice::u8 alpha = 255;
    };

    template<>
    struct ColorData<ColorFormat::LinearRGB>
    {
        ice::f32 red;
        ice::f32 green;
        ice::f32 blue;
        ice::f32 alpha = 1.0f;

        constexpr auto gammut_clipped() const noexcept -> ColorData<ColorFormat::LinearRGB>;

        constexpr auto to_oklab() const noexcept -> ColorData<ColorFormat::OkLAB>;
        inline auto to_oklch() const noexcept -> ColorData<ColorFormat::OkLCH>;

        constexpr auto to_srgb() const noexcept -> ColorData<ColorFormat::StandardRGB>;
        constexpr auto to_u8() const noexcept -> ColorData<ColorFormat::LinearRGBu8>;
    };

    template<>
    struct ColorData<ColorFormat::StandardRGB>
    {
        ice::f32 red;
        ice::f32 green;
        ice::f32 blue;
        ice::f32 alpha = 1.0f;

        constexpr auto to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>;
    };

    template<>
    struct ColorData<ColorFormat::OkLAB>
    {
        ice::f32 lightness;
        ice::f32 a;
        ice::f32 b;
        ice::f32 alpha = 1.0f;

        constexpr auto gammut_corrected(
            ice::ColorSpace color_space = ColorSpace::SRGB,
            ice::f32 coefficient = 0.5
        ) const noexcept -> ColorData<ColorFormat::OkLAB>;

        constexpr auto to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>;
        inline auto to_oklch() const noexcept -> ColorData<ColorFormat::OkLCH>;
    };

    template<>
    struct ColorData<ColorFormat::OkLCHu8>
    {
        ice::u8 hue128_lightness; // 0 - 100 [7 bits]
        ice::u8 chroma; // 0 - 0.41 (maximum depends on lightness, 47 is general max for rc2020, 37 for P3, 32 for sRGB)
        ice::u8 hue; // 0 - 359 (0 == 360) [9 bits]
        ice::u8 alpha = 255;

        constexpr auto to_f32() const noexcept -> ColorData<ColorFormat::OkLCH>;
    };

    template<>
    struct ColorData<ColorFormat::OkLCH>
    {
        ice::f32 lightness; // 0 - 100 [7 bits]
        ice::f32 chroma; // 0 - 0.41 (maximum depends on lightness, 47 is general max for rc2020, 37 for P3, 32 for sRGB)
        ice::deg32 hue; // 0 - 359 (0 == 360) [9 bits]
        ice::f32 alpha = 1.0f;

        // Helpers
        constexpr auto with_hue(ice::deg32 hue) const noexcept -> ColorData<ColorFormat::OkLCH>;
        constexpr auto brightened(ice::f32 step = 0.05f) const noexcept -> ColorData<ColorFormat::OkLCH>;
        constexpr auto darkened(ice::f32 step = 0.05f) const noexcept -> ColorData<ColorFormat::OkLCH>;
        constexpr auto saturated(ice::f32 step = 0.02f, ice::f32 max_chroma = 0.32f) const noexcept -> ColorData<ColorFormat::OkLCH>;
        constexpr auto desaturated(ice::f32 step = 0.02f, ice::f32 max_chroma = 0.32f) const noexcept -> ColorData<ColorFormat::OkLCH>;

        constexpr auto gammut_corrected(
            ice::ColorSpace color_space = ColorSpace::SRGB,
            ice::f32 coefficient = 0.5
        ) const noexcept -> ColorData<ColorFormat::OkLCH>;

        // Conversions
        constexpr auto to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>;
        constexpr auto to_oklab() const noexcept -> ColorData<ColorFormat::OkLAB>;

        constexpr auto to_u8() const noexcept -> ColorData<ColorFormat::OkLCHu8>;
    };

    // LINEAR RGB - BEGIN

    constexpr auto ColorData<ColorFormat::LinearRGB>::gammut_clipped() const noexcept -> ColorData<ColorFormat::LinearRGB>
    {
        return {
            std::clamp(this->red, 0.0f, 1.0f),
            std::clamp(this->green, 0.0f, 1.0f),
            std::clamp(this->blue, 0.0f, 1.0f),
            this->alpha,
        };
    }

    constexpr auto ColorData<ColorFormat::LinearRGB>::to_oklab() const noexcept -> ColorData<ColorFormat::OkLAB>
    {
        ice::f32 const L = 0.4122214708f * this->red + 0.5363325363f * this->green + 0.0514459929f * this->blue;
        ice::f32 const M = 0.2119034982f * this->red + 0.6806995451f * this->green + 0.1073969566f * this->blue;
        ice::f32 const S = 0.0883024619f * this->red + 0.2817188376f * this->green + 0.6299787005f * this->blue;

        ice::f32 const l3r = ice::math::cbrt(L);
        ice::f32 const m3r = ice::math::cbrt(M);
        ice::f32 const s3r = ice::math::cbrt(S);

        ice::f32 const oklab_lightness = 0.2104542553f * l3r + 0.7936177850f * m3r - 0.0040720468f * s3r;
        ice::f32 const oklab_a = 1.9779984951f * l3r - 2.4285922050f * m3r + 0.4505937099f * s3r;
        ice::f32 const oklab_b = 0.0259040371f * l3r + 0.7827717662f * m3r - 0.8086757660f * s3r;

        return { oklab_lightness, oklab_a, oklab_b, this->alpha };
    }

    inline auto ColorData<ColorFormat::LinearRGB>::to_oklch() const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        return to_oklab().to_oklch();
    }

    constexpr auto ColorData<ColorFormat::LinearRGB>::to_srgb() const noexcept -> ColorData<ColorFormat::StandardRGB>
    {
        ice::f32 const r = ice::math::clamp(ice::detail::linear_to_srgb(this->red), 0.0, 1.0);
        ice::f32 const g = ice::math::clamp(ice::detail::linear_to_srgb(this->green), 0.0, 1.0);
        ice::f32 const b = ice::math::clamp(ice::detail::linear_to_srgb(this->blue), 0.0, 1.0);
        return { r, g, b, this->alpha };
    }

    constexpr auto ColorData<ColorFormat::LinearRGB>::to_u8() const noexcept -> ColorData<ColorFormat::LinearRGBu8>
    {
        ColorData<ColorFormat::LinearRGB> const clipped = gammut_clipped();
        return {
            ice::u8(clipped.red * 255.f + 0.5f),
            ice::u8(clipped.green * 255.f + 0.5f),
            ice::u8(clipped.blue * 255.f + 0.5f),
            ice::u8(clipped.alpha * 255.f + 0.5f),
        };
    }

    // LINEAR RGB - END
    //
    // STANDARD RGB - BEGIN

    constexpr auto ColorData<ColorFormat::StandardRGB>::to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>
    {
        ice::f32 const r = ice::detail::srgb_to_linear(this->red);
        ice::f32 const g = ice::detail::srgb_to_linear(this->green);
        ice::f32 const b = ice::detail::srgb_to_linear(this->blue);
        return { r, g, b, this->alpha };
    }

    // STANDARD RGB - END
    //
    // OKLAB - BEGIN

    constexpr auto ColorData<ColorFormat::OkLAB>::gammut_corrected(
        ice::ColorSpace color_space,
        ice::f32 coefficient
    ) const noexcept -> ColorData<ColorFormat::OkLAB>
    {
        // TODO: Other color spaces. SRGB by default for all

        ice::f32 const L = this->lightness;
        ice::f32 const C = std::max(ice::f32_eps, ice::math::sqrt(this->a * this->a + this->b * this->b));
        ice::f32 const a_ = this->a / C;
        ice::f32 const b_ = this->b / C;

        ice::detail::OkLCH_HueCusp const cusp = find_cusp(a_, b_);

        ice::f32 const Ld = L - 0.5f;
        ice::f32 const e1 = 0.5f + ice::math::abs(Ld) + coefficient * C;
        ice::f32 const L0 = 0.5f * (1.f + ice::math::sgn(Ld)*(e1 - ice::math::sqrt(e1*e1 - 2.f * ice::math::abs(Ld))));

        ice::f32 const t = ice::detail::find_gamut_intersection(cusp, L, C, L0);
        ice::f32 const L_clipped = L0 * (1.f - t) + t * L;
        ice::f32 const C_clipped = t * C;
        if (C_clipped >= C)
        {
            return *this;
        }
        return { L_clipped, C_clipped * a_, C_clipped * b_, this->alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLAB>::to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>
    {
        // OKLab → LMS
        ice::f32 const l2 = (this->lightness * 1.0f) + (this->a * 0.3963377774f) + (this->b * 0.2158037573f);
        ice::f32 const m2 = (this->lightness * 1.0f) + (this->a * -0.1055613458f) + (this->b * -0.0638541728f);
        ice::f32 const s2 = (this->lightness * 1.0f) + (this->a * -0.0894841775f) + (this->b * -1.291485548f);

        // Cube
        ice::f32 const l3 = l2 * l2 * l2;
        ice::f32 const m3 = m2 * m2 * m2;
        ice::f32 const s3 = s2 * s2 * s2;

        ice::f32 const r_lin = (+4.0767416621f * l3) - (3.3077115913f * m3) + (0.2309699292f * s3);
        ice::f32 const g_lin = (-1.2684380046f * l3) + (2.6097574011f * m3) - (0.3413193965f * s3);
        ice::f32 const b_lin = (-0.0041960863f * l3) - (0.7034186147f * m3) + (1.7076147010f * s3);

        // Oklch to Oklab
        return { r_lin, g_lin, b_lin, this->alpha };
    }

    inline auto ColorData<ColorFormat::OkLAB>::to_oklch() const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        ice::f32 const oklch_lightness = this->lightness;
        ice::f32 const oklch_c = ice::math::sqrt(this->a * this->a + this->b * this->b);
        ice::f32 const oklch_ht = ice::math::atan2(this->b, this->a) * (180.0f / 3.14159265358979323846f);
        ice::f32 const oklch_h = oklch_ht < 0.0f ? oklch_ht + 360.f : oklch_ht;
        return { oklch_lightness, ice::max(ice::f32_eps, oklch_c), oklch_h, this->alpha };
    }

    // OKLAB - END
    //
    // OKLCH (compressed) - BEGIN

    constexpr auto ColorData<ColorFormat::OkLCHu8>::to_f32() const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        ice::f32 const lightness_f32 = ice::f32((hue128_lightness & 0x7f) * 0.01f);
        ice::f32 const hue_f32 = ice::f32(hue) + ice::f32(ice::u16(hue128_lightness & 0x80) << 1);
        return {
            lightness_f32,
            ice::f32(chroma / 510.f), // We only need to max up to 0.5 (for safety we go above the max of 0.47) so we multiply by 2 x 255
            hue_f32,
            ice::f32(alpha / 255.f)
        };
    }

    // OKLCH (compressed) - END
    //
    // OKLCH - BEGIN

    constexpr auto ColorData<ColorFormat::OkLCH>::with_hue(ice::deg32 new_hue) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        ice::u32 const floored = ice::u32(new_hue._value);
        ice::f32 const reminder = new_hue._value - ice::f32(floored);
        return { lightness, chroma, ice::f32(floored % 360) + reminder, alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::brightened(ice::f32 step) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        return { std::clamp(lightness + step, 0.f, 1.0f), chroma, hue, alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::darkened(ice::f32 step) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        return brightened(-step);
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::saturated(ice::f32 step, ice::f32 max_chroma) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        return { lightness, std::clamp(chroma + step, ice::f32_eps, max_chroma), hue, alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::desaturated(ice::f32 step, ice::f32 max_chroma) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        return saturated(-step, max_chroma);
    }


    constexpr auto ColorData<ColorFormat::OkLCH>::gammut_corrected(
        ice::ColorSpace color_space,
        ice::f32 coefficient
    ) const noexcept -> ColorData<ColorFormat::OkLCH>
    {
        // TODO: Other color spaces. SRGB by default for all

        ice::f32 const L = this->lightness;
        ice::f32 const C = std::max(ice::f32_eps, this->chroma);

        ice::detail::OkLCH_HueCusp const ab_cusp = find_cusp_ch(this->chroma, ice::radians(this->hue));

        ice::f32 const Ld = L - 0.5f;
        ice::f32 const e1 = 0.5f + ice::math::abs(Ld) + coefficient * C;
        ice::f32 const L0 = 0.5f * (1.f + ice::math::sgn(Ld) * (e1 - ice::math::sqrt(e1 * e1 - 2.f * ice::math::abs(Ld))));

        ice::f32 const t = ice::detail::find_gamut_intersection(ab_cusp, L, C, L0);
        ice::f32 const L_clipped = L0 * (1.f - t) + t * L;
        ice::f32 const C_clipped = t * C;

        if (C_clipped >= C)
        {
            return *this;
        }
        return { L_clipped, C_clipped, this->hue, this->alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::to_lrgb() const noexcept -> ColorData<ColorFormat::LinearRGB>
    {
        return to_oklab().to_lrgb();
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::to_oklab() const noexcept -> ColorData<ColorFormat::OkLAB>
    {
        ice::f32 const loc_chroma = std::max<ice::f32>(ice::f32_eps, this->chroma);
        ice::f32 const oklab_lightness = this->lightness;
        ice::f32 const oklab_a = loc_chroma * ice::math::cos(rad{ this->hue.to_rad32() });
        ice::f32 const oklab_b = loc_chroma * ice::math::sin(rad{ this->hue.to_rad32() });
        return { oklab_lightness, oklab_a, oklab_b, this->alpha };
    }

    constexpr auto ColorData<ColorFormat::OkLCH>::to_u8() const noexcept -> ColorData<ColorFormat::OkLCHu8>
    {
        ice::u16 const hue_16 = ice::u16(hue.raw_value()) & 0x01'ff;
        ice::u8 const lightness_u8 = ice::u8(lightness * 100) | (ice::u8(hue_16 >> 1) & 0x80);
        return {
            lightness_u8,
            ice::u8(chroma * 510.f + 0.5f), // We only need to max up to 0.5 (for safety we go above the max of 0.47) so we multiply by 2 x 255
            ice::u8(hue_16),
            ice::u8(alpha * 255.f + 0.5f)
        };
    }

    // OKLCH - END

} // namespace ice
