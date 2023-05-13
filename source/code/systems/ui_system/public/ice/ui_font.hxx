/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/font.hxx>

namespace ice::ui
{

    struct FontInfo
    {
        ice::u32 font_name_offset;
        ice::u32 font_name_size;
        ice::u16 font_size;

        ice::u16 resource_i;
    };

} // namespace ice::ui
