#pragma once
#include <ice/ui_types.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    struct UIData
    {
        ice::Span<ice::ui::ElementInfo const> elements;
        ice::Span<ice::ui::Size const> sizes;
        ice::Span<ice::ui::Position const> positions;
        ice::Span<ice::ui::RectOffset const> margins;
        ice::Span<ice::ui::RectOffset const> paddings;
    };

} // namespace ice::ui
