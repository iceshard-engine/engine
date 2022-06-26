#pragma once
#include <ice/ui_types.hxx>
#include <ice/font.hxx>

namespace ice::ui
{

    struct UIFont
    {
        ice::Font const* font;
        ice::u32 size;
        // bool is_bold;
        // bool is_italic;
    };

    struct UIData
    {
        ice::Span<ice::ui::UIFont const> fonts;
        ice::Span<ice::ui::ElementInfo const> elements;
        ice::Span<ice::ui::Size const> sizes;
        ice::Span<ice::ui::Position const> positions;
        ice::Span<ice::ui::RectOffset const> margins;
        ice::Span<ice::ui::RectOffset const> paddings;
        ice::Span<ice::ui::ButtonInfo const> data_buttons;
        void const* additional_data;
    };

} // namespace ice::ui
