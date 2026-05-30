/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/math.hxx>
#include <ice/math/algorithm.hxx>
#include <ice/detail/color_enums.hxx>
#include <ice/profiler.hxx>
#include <array>

namespace ice::detail
{

    //! \brief Holds information where the 'lightness' and 'chroma' are at the highest point of the OkLCH
    //!   color curve.
    //! \note "Cusp" is defined as "sharp corner" or "suddent bend" according to searches.
    struct OkLCH_HueCusp
    {
        ice::f32 a, b;
        ice::f32 lightness;
        ice::f32 chroma;
    };

    // Linear to sRGB
    // Condition	Value
    // 0 ≤ L ≤ 0.0031308	S = L * 12.92
    // 0.0031308 < L ≤ 1	S = 1.055 * L1/2.4 - 0.055
    constexpr auto linear_to_srgb(ice::f32 x) noexcept -> ice::f32
    {
        if (x <= 0.0031308f)
        {
            return 12.92f * x;
        }
        else
        {
            return 1.055f * ice::math::pow(x, 1.0f / 2.4f) - 0.055f;
        }
    }

    // sRGB to Linear
    // Condition	Value
    // 0 ≤ S ≤ 0.04045	L = S/12.92
    // 0.04045 < S ≤ 1	L = ((S+0.055)/1.055)2.4
    constexpr auto srgb_to_linear(ice::f32 x) noexcept -> ice::f32
    {
        if (x <= 0.04045f)
        {
            return x / 12.92f;
        }
        else
        {
            return ice::math::pow((x + 0.055f) / 1.055f, 2.4f);
        }
    }

    // FROM: https://bottosson.github.io/posts/oklab/
    // FROM: https://bottosson.github.io/posts/gamutclipping/
    //
    // Copyright (c) 2021 Björn Ottosson
    //
    // Permission is hereby granted, free of charge, to any person obtaining a copy of
    // this software and associated documentation files (the "Software"), to deal in
    // the Software without restriction, including without limitation the rights to
    // use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    // of the Software, and to permit persons to whom the Software is furnished to do
    // so, subject to the following conditions:
    //
    // The above copyright notice and this permission notice shall be included in all
    // copies or substantial portions of the Software.
    //
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    // SOFTWARE.

    /// from: https://bottosson.github.io/posts/oklab/
    constexpr auto from_oklab_to_lrgb(ice::math::vec3f from) noexcept -> ice::math::vec3f
    {
        IPT_ZONE_SCOPED;
        // OKLab → LMS
        ice::f32 const l2 = (from.v[0][0] * 1.0f) + (from.v[0][1] * 0.3963377774f) + (from.v[0][2] * 0.2158037573f);
        ice::f32 const m2 = (from.v[0][0] * 1.0f) + (from.v[0][1] * -0.1055613458f) + (from.v[0][2] * -0.0638541728f);
        ice::f32 const s2 = (from.v[0][0] * 1.0f) + (from.v[0][1] * -0.0894841775f) + (from.v[0][2] * -1.291485548f);

        // Cube
        ice::f32 const l3 = l2 * l2 * l2;
        ice::f32 const m3 = m2 * m2 * m2;
        ice::f32 const s3 = s2 * s2 * s2;

        ice::f32 const r_lin = (+4.0767416621f * l3) - (3.3077115913f * m3) + (0.2309699292f * s3);
        ice::f32 const g_lin = (-1.2684380046f * l3) + (2.6097574011f * m3) - (0.3413193965f * s3);
        ice::f32 const b_lin = (-0.0041960863f * l3) - (0.7034186147f * m3) + (1.7076147010f * s3);

        // Oklch to Oklab
        return { r_lin, g_lin, b_lin };
    }

    //! \brief Compute max saturation for sRGB.
    //! \details Finds the maximum saturation possible for a given hue that fits in sRGB.
    //!   Saturation here is defined as 'S = C/L', 'a' and 'b' must be normalized so that 'a^2 + b^2 == 1'
    //! \note Implementation copied from: https://bottosson.github.io/posts/gamutclipping/
    constexpr auto compute_max_saturation(ice::f32 a, ice::f32 b) noexcept -> ice::f32
    {
        // Max saturation will be when one of r, g or b goes below zero.
        // Select different coefficients depending on which component goes below zero first
        ice::f32 k0, k1, k2, k3, k4, wl, wm, ws;

        if (-1.88170328f * a - 0.80936493f * b > 1)
        {
            // Red component
            k0 = +1.19086277f; k1 = +1.76576728f; k2 = +0.59662641f; k3 = +0.75515197f; k4 = +0.56771245f;
            wl = +4.0767416621f; wm = -3.3077115913f; ws = +0.2309699292f;
        }
        else if (1.81444104f * a - 1.19445276f * b > 1)
        {
            // Green component
            k0 = +0.73956515f; k1 = -0.45954404f; k2 = +0.08285427f; k3 = +0.12541070f; k4 = +0.14503204f;
            wl = -1.2684380046f; wm = +2.6097574011f; ws = -0.3413193965f;
        }
        else
        {
            // Blue component
            k0 = +1.35733652f; k1 = -0.00915799f; k2 = -1.15130210f; k3 = -0.50559606f; k4 = +0.00692167f;
            wl = -0.0041960863f; wm = -0.7034186147f; ws = +1.7076147010f;
        }

        // Approximate max saturation using a polynomial:
        ice::f32 const s_initial = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

        // Do one step Halley's method to get closer
        // this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
        // this should be sufficient for most applications, otherwise do two/three steps
        ice::f32 const k_l = +0.3963377774f * a + 0.2158037573f * b;
        ice::f32 const k_m = -0.1055613458f * a - 0.0638541728f * b;
        ice::f32 const k_s = -0.0894841775f * a - 1.2914855480f * b;

        {
            ice::f32 const l_ = 1.f + s_initial * k_l;
            ice::f32 const m_ = 1.f + s_initial * k_m;
            ice::f32 const s_ = 1.f + s_initial * k_s;

            ice::f32 const l = l_ * l_ * l_;
            ice::f32 const m = m_ * m_ * m_;
            ice::f32 const s = s_ * s_ * s_;

            ice::f32 const l_ds = 3.f * k_l * l_ * l_;
            ice::f32 const m_ds = 3.f * k_m * m_ * m_;
            ice::f32 const s_ds = 3.f * k_s * s_ * s_;

            ice::f32 const l_ds2 = 6.f * k_l * k_l * l_;
            ice::f32 const m_ds2 = 6.f * k_m * k_m * m_;
            ice::f32 const s_ds2 = 6.f * k_s * k_s * s_;

            ice::f32 const f  = wl * l     + wm * m     + ws * s;
            ice::f32 const f1 = wl * l_ds  + wm * m_ds  + ws * s_ds;
            ice::f32 const f2 = wl * l_ds2 + wm * m_ds2 + ws * s_ds2;

            return s_initial - f * f1 / (f1*f1 - 0.5f * f * f2);
        }
    }

    //! \brief Finds L_cusp and C_cusp for a given 'a' and 'b' values of OKLAB color.
    //! \details Argument 'a' and 'b' must be normalized so a^2 + b^2 == 1
    constexpr auto find_cusp(ice::f32 a, ice::f32 b) noexcept -> ice::detail::OkLCH_HueCusp
    {
        IPT_ZONE_SCOPED;
        // First, find the maximum saturation (saturation S = C/L)
        ice::f32 const s_cusp = compute_max_saturation(a, b);

        // Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
        ice::vec3f const rgb_at_max = ice::detail::from_oklab_to_lrgb({ 1, s_cusp * a, s_cusp * b });
        ice::f32 const l_cusp = ice::math::cbrt(
            1.f / ice::max_of(rgb_at_max.v[0][0], rgb_at_max.v[0][1], rgb_at_max.v[0][2])
        );

        ice::f32 const c_cusp = l_cusp * s_cusp;
        return { a, b, l_cusp , c_cusp };
    }

    //! \brief Finds L_cusp and C_cusp for a given 'a' and 'b' values of OKLAB color.
    //! \details Argument 'a' and 'b' must be normalized so a^2 + b^2 == 1
    constexpr auto find_cusp_ch(ice::f32 chroma, ice::rad hue) noexcept -> ice::detail::OkLCH_HueCusp
    {
        ice::f32 const C = std::max(ice::f32_eps, chroma);
        ice::f32 const oklab_a = chroma * ice::math::cos(hue);
        ice::f32 const oklab_b = chroma * ice::math::sin(hue);
        ice::f32 const oklab_a_ = oklab_a / C;
        ice::f32 const oklab_b_ = oklab_b / C;

        return find_cusp(oklab_a_, oklab_b_);
    }

#if 0 // The lookup into the table can be much slower than calculating this due to reaching into an uncached memory location.
    static constexpr std::array<ice::vec2f, 3600> Constant_HueCuspTable = []() noexcept -> std::array<ice::vec2f, 3600>
        {
            ice::deg hue = { 0.0f };
            ice::f32 const hue_last = hue.value - ice::f32_eps;
            std::array<ice::vec2f, 3600> result{};
            while (hue.value < hue_last)
            {
                ice::f32 const a = ice::f32_eps * ice::cos(ice::math::radians(hue));
                ice::f32 const b = ice::f32_eps * ice::sin(ice::math::radians(hue));

                ice::f32 const C = ice::max(ice::f32_eps, ice::sqrt(a * a + b * b));
                ice::f32 const a_ = a / C;
                ice::f32 const b_ = b / C;

                OkLCH_HueCusp const cusp = find_cusp(a_, b_);
                result[ice::u32(hue.value * 10.f)] = { cusp.lightness, cusp.chroma };
                hue.value += 0.1f;
            }

            return result;
        }();

    //! \brief Finds L_cusp and C_cusp for a given hue.
    //! \details Argument 'a' and 'b' must be normalized so a^2 + b^2 == 1
    constexpr auto find_cusp(ice::deg hue) noexcept -> ice::vec2f
    {
        IPT_ZONE_SCOPED;
        return Constant_HueCuspTable[ice::u32(hue.value * 10.f)];
    }
#endif

    // Finds intersection of the line defined by
    // L = L0 * (1 - t) + t * L1;
    // C = t * C1;
    // a and b must be normalized so a^2 + b^2 == 1
    //! \note Implementation copied from: https://bottosson.github.io/posts/gamutclipping/
    constexpr auto find_gamut_intersection(
        ice::detail::OkLCH_HueCusp cusp,
        ice::f32 L1,
        ice::f32 C1,
        ice::f32 L0
    ) noexcept -> ice::f32
    {
        // Find the intersection for upper and lower half seprately
        ice::f32 t = 0.0f;
        if (((L1 - L0) * cusp.chroma - (cusp.lightness - L0) * C1) <= 0.f)
        {
            // Lower half
            t = cusp.chroma * L0 / (C1 * cusp.lightness + cusp.chroma * (L0 - L1));
        }
        else
        {
            // Upper half

            // First intersect with triangle
            t = cusp.chroma * (L0 - 1.f) / (C1 * (cusp.lightness - 1.f) + cusp.chroma * (L0 - L1));

            // Then one step Halley's method
            {
                ice::f32 const dL = L1 - L0;
                ice::f32 const dC = C1;

                ice::f32 const k_l = +0.3963377774f * cusp.a + 0.2158037573f * cusp.b;
                ice::f32 const k_m = -0.1055613458f * cusp.a - 0.0638541728f * cusp.b;
                ice::f32 const k_s = -0.0894841775f * cusp.a - 1.2914855480f * cusp.b;

                ice::f32 const l_dt = dL + dC * k_l;
                ice::f32 const m_dt = dL + dC * k_m;
                ice::f32 const s_dt = dL + dC * k_s;


                // If higher accuracy is required, 2 or 3 iterations of the following block can be used:
                //for (ice::u32 iter = 0; iter < 3; ++iter)
                {
                    ice::f32 const L = L0 * (1.f - t) + t * L1;
                    ice::f32 const C = t * C1;

                    ice::f32 const l_ = L + C * k_l;
                    ice::f32 const m_ = L + C * k_m;
                    ice::f32 const s_ = L + C * k_s;

                    ice::f32 const l = l_ * l_ * l_;
                    ice::f32 const m = m_ * m_ * m_;
                    ice::f32 const s = s_ * s_ * s_;

                    ice::f32 const ldt = 3 * l_dt * l_ * l_;
                    ice::f32 const mdt = 3 * m_dt * m_ * m_;
                    ice::f32 const sdt = 3 * s_dt * s_ * s_;

                    ice::f32 const ldt2 = 6 * l_dt * l_dt * l_;
                    ice::f32 const mdt2 = 6 * m_dt * m_dt * m_;
                    ice::f32 const sdt2 = 6 * s_dt * s_dt * s_;

                    ice::f32 const r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1;
                    ice::f32 const r1 = 4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
                    ice::f32 const r2 = 4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

                    ice::f32 const u_r = r1 / (r1 * r1 - 0.5f * r * r2);
                    ice::f32 const t_r = -r * u_r;

                    ice::f32 const g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1;
                    ice::f32 const g1 = -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
                    ice::f32 const g2 = -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

                    ice::f32 const u_g = g1 / (g1 * g1 - 0.5f * g * g2);
                    ice::f32 const t_g = -g * u_g;

                    ice::f32 const b0 = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1;
                    ice::f32 const b1 = -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
                    ice::f32 const b2 = -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

                    ice::f32 const u_b = b1 / (b1 * b1 - 0.5f * b0 * b2);
                    ice::f32 const t_b = -b0 * u_b;

                    ice::f32 const t_r2 = u_r >= 0.f ? t_r : ice::f32_max;
                    ice::f32 const t_g2 = u_g >= 0.f ? t_g : ice::f32_max;
                    ice::f32 const t_b2 = u_b >= 0.f ? t_b : ice::f32_max;

                    t += ice::min_of(t_r2, t_g2, t_b2);
                }
            }
        }

        return t;
    }

} // namespace ice::detail
