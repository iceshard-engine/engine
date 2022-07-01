#pragma once
#include <ice/string.hxx>

namespace ice::ui
{

    struct ButtonInfo
    {
        ice::usize text_offset;
        ice::u32 text_size;

        ice::u16 action_text_i;
        ice::u16 action_on_click_i;
    };

} // namespace ice::ui
