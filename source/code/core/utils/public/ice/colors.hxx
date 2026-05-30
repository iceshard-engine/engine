/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/detail/color_data.hxx>

namespace ice
{

    namespace color
    {

        using OkLAB = ice::detail::ColorData<ColorFormat::OkLAB>;
        using OkLCH = ice::detail::ColorData<ColorFormat::OkLCH>;
        using OkLCHu8 = ice::detail::ColorData<ColorFormat::OkLCHu8>;
        using LRGB = ice::detail::ColorData<ColorFormat::LinearRGB>;
        using LRGBu8 = ice::detail::ColorData<ColorFormat::LinearRGBu8>;
        using SRGB = ice::detail::ColorData<ColorFormat::StandardRGB>;

        // Static defined colors
        static constexpr OkLCH White{ 1.0f, 0.0f, 0.0_deg, 1.0f };
        static constexpr OkLCH Black{ 0.0f, 0.0f, 0.0_deg, 1.0f };

        static constexpr OkLCH Red{ 0.628f, 0.2577f, 29.23f, 1.0f };
        static constexpr OkLCH Green{ 0.8664f, 0.294827f, 142.4953f, 1.0f };
        static constexpr OkLCH Blue{ 0.452f, 0.313214f, 264.052f, 1.0f };

        static constexpr OkLCH Yellow{ 0.968f, 0.211f, 109.77f, 1.0f };
        static constexpr OkLCH Magenta{ 0.7017f, 0.3225f, 328.36f, 1.0f };
        static constexpr OkLCH Cyan{ 0.9054f, 0.15455f, 194.769f, 1.0f };

    } // namespace color

    using Color = ice::color::OkLCH;
    using ShaderColor = ice::color::LRGB;

} // namespace ice
