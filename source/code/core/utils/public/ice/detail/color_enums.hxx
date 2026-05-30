/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class ColorSpace : ice::u8
    {
        SRGB,
        // P3D65,
        // RC2020
    };

    enum class ColorFormat : ice::u8
    {
        // Supported color formats
        OkLCH,
        OkLAB, // Implemented for functional reasons, mostly unused directly.
        LinearRGB, // Most common shader compatible color
        StandardRGB, // Implemented for debugging purposes? Not sure if needed.

        // uint8 version of some color formats to pass to shaders
        OkLCHu8,
        LinearRGBu8,
    };

} // namespace ice
