#pragma once
#include <ice/ui_types.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_element_draw.hxx>

namespace ice::ui
{

    struct Element
    {
        ice::ui::ElementInfo const* definition;

        ice::ui::Rect bbox;
        ice::ui::Rect hitbox;
        ice::ui::DrawData draw_data;
        ice::ui::Position draw_offset;

        bool center_vertical : 1;
        bool center_horizontal : 1;
    };


} // namespace ice::ui
