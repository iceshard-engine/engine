#pragma once
#include <ice/string.hxx>

namespace ice::ui
{

    struct RawButtonInfo
    {
        ice::Utf8String text;
    };

    struct ButtonInfo
    {
        ice::usize text_offset;
        ice::u32 text_size;
    };

} // namespace ice::ui
