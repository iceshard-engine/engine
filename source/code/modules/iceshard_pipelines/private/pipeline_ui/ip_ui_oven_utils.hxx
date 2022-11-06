/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "ip_ui_oven.hxx"
#include "ip_ui_oven_types.hxx"

namespace ice
{

    static constexpr ice::String Constant_UIAttribute_Size = "size";
    static constexpr ice::String Constant_UIAttribute_Position = "position";
    static constexpr ice::String Constant_UIAttribute_Margin = "margin";
    static constexpr ice::String Constant_UIAttribute_Padding = "padding";
    static constexpr ice::String Constant_UIAttribute_Style = "style";

    static constexpr ice::String Constant_UIAttributeKeyword_Auto = "auto";
    static constexpr ice::String Constant_UIAttributeKeyword_Stretch = "*";

    auto xml_first_node(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns = { },
        ice::String name = { }
    ) noexcept -> rapidxml_ns::xml_node<char> const*;

    auto xml_next_sibling(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns = { },
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
    ) noexcept -> ice::String;

    auto xml_value(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::String;

    auto xml_value_noutf8(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::String;

    void parse_element_size(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Size& out_size
    ) noexcept;

    void parse_element_pos(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Position& out_pos
    ) noexcept;

    void parse_element_offset(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::RectOffset& out_offset
    ) noexcept;

    void parse_element_color(
        ice::String value,
        ice::ui::StyleColor& out_color
    ) noexcept;

} // namespace ice
