#pragma once
#include "ip_ui_oven.hxx"

namespace ice
{

    static constexpr ice::String Constant_UIAttribute_Size = "size";
    static constexpr ice::String Constant_UIAttribute_Position = "position";
    static constexpr ice::String Constant_UIAttribute_Margin = "margin";
    static constexpr ice::String Constant_UIAttribute_Padding = "padding";

    static constexpr ice::String Constant_UIAttributeKeyword_Auto = "auto";
    static constexpr ice::String Constant_UIAttributeKeyword_Stretch = "*";

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
