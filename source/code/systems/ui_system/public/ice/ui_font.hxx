#pragma once
#include <ice/font.hxx>

namespace ice::ui
{

    struct FontInfo
    {
        ice::usize font_name_offset;
        ice::u32 font_name_size;
        ice::u16 font_size;

        ice::u16 resource_i;
    };

} // namespace ice::ui
