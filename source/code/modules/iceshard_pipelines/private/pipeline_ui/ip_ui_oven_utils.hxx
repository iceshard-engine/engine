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

    auto xml_first_node(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns,
        ice::String name = { }
    ) noexcept -> rapidxml_ns::xml_node<char> const*;

    auto xml_next_sibling(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns,
        ice::String name = { }
    ) noexcept -> rapidxml_ns::xml_node<char> const*;

    auto xml_first_attrib(
        rapidxml_ns::xml_node<char> const* node,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_attribute<char> const*;

    auto xml_next_attrib(
        rapidxml_ns::xml_attribute<char> const* attrib,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_attribute<char> const*;

    auto xml_name(
        rapidxml_ns::xml_node<char> const* node
    ) noexcept -> ice::String;

    auto xml_name(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::String;

    auto xml_value(
        rapidxml_ns::xml_node<char> const* node
    ) noexcept -> ice::Utf8String;

    auto xml_value(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::Utf8String;

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
