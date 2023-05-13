/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/ui_types.hxx>
#include <ice/ui_data_ref.hxx>

namespace ice::ui
{

    enum class StyleFlags : ice::u32
    {
        None = 0x0,

        BackgroundColor = 0x0000'0001,
        BackgroundTexture = 0x0000'0002,

        ForegroundColor = 0x0000'0010,
        ForegroundTexture = 0x0000'0020,

        TargetBackground = BackgroundColor | BackgroundTexture,
        TargetForeground = ForegroundColor | ForegroundTexture,

        All = TargetBackground | TargetForeground
    };

    struct StyleColor
    {
        ice::f32 red;
        ice::f32 green;
        ice::f32 blue;
        ice::f32 alpha;
    };

    struct StyleTexture
    {
        ice::u16 resource_i;
        ice::u16 alpha;
        ice::vec2f uvs[4];
    };

    struct StyleInfo
    {
        ice::ui::ElementState target_state;
        ice::ui::StyleFlags flags;
        ice::ui::DataRef data_bg;
        ice::ui::DataRef data_fg;
        ice::ui::DataRef data_border;
    };

} // namespace ice::ui
