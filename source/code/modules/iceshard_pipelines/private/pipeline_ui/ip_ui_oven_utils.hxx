#pragma once
#include "ip_ui_oven.hxx"

namespace ice
{

    void parse_element_size(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Size& size
    ) noexcept;

    void parse_element_pos(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Position& pos
    ) noexcept;

    void parse_element_offset(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::RectOffset& offset
    ) noexcept;

} // namespace ice
