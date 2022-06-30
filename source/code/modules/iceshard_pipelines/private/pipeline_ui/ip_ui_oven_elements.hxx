#pragma once
#include <ice/ui_action.hxx>

#include "ip_ui_oven.hxx"

namespace ice
{

    struct RawButtonInfo
    {
        struct Action
        {
            ice::ui::ActionType type_id;
            ice::ui::ActionData type_value;
            ice::Utf8String action_id;
            ice::Utf8String action_value;
        };

        ice::Utf8String text;
        Action action_on_click;
    };

    void compile_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::u16 parent_idx,
        ice::pod::Array<RawElement>& elements
    ) noexcept;

} // namespace ice
