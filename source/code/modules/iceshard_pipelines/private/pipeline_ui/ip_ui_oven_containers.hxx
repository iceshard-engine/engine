#pragma once
#include <ice/ui_action.hxx>

#include "ip_ui_oven.hxx"

namespace ice
{
    static constexpr ice::String Constant_UIElement_Layout = "layout";
    static constexpr ice::String Constant_UIAttribute_LayoutType = "direction";

    static constexpr ice::Utf8String Constant_UIAttributeKeyword_Vertical = u8"vertical";
    static constexpr ice::Utf8String Constant_UIAttributeKeyword_Horizontal = u8"horizontal";

    struct RawLayoutInfo
    {

    };

    void parse_layout_element(
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::Allocator& alloc,
        ice::RawElement& info
    ) noexcept;

} // namespace ice
