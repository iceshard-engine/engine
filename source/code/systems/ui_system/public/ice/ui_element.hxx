#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class ElementType : ice::u32
    {
        Page,
        ListBox,
        Button,
        Label,
        Custom0,
    };

    struct Element
    {
        ice::ui::ElementType type;
    };

} // namespace ice::ui
