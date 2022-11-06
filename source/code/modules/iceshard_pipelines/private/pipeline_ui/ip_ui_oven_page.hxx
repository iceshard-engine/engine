/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/shard.hxx>
#include <ice/container/array.hxx>
#include <ice/ui_style.hxx>

#include "ip_ui_oven.hxx"

namespace ice
{

    static constexpr ice::String Constant_UIElementGroup_Resources = "resources";
    static constexpr ice::String Constant_UIElementGroup_Shards = "shards";
    static constexpr ice::String Constant_UIElementGroup_Styles = "styles";

    static constexpr ice::String Constant_UIElement_Resource = "resource";
    static constexpr ice::String Constant_UIElement_Shard = "shard";
    static constexpr ice::String Constant_UIElement_Style = "style";
    static constexpr ice::String Constant_UIElement_Page = "page";

    static constexpr ice::String Constant_UIElement_StyleBackgroud = "background";

    static constexpr ice::String Constant_UIAttribute_ResourceType = "type";
    static constexpr ice::String Constant_UIAttribute_ResourceName = "name";
    static constexpr ice::String Constant_UIAttribute_ResourceFontSize = "size";
    static constexpr ice::String Constant_UIAttribute_ResourceFontName = "font";

    static constexpr ice::String Constant_UIAttribute_ShardName = "name";
    static constexpr ice::String Constant_UIAttribute_ShardAction = "action";

    static constexpr ice::String Constant_UIAttribute_StyleName = "name";
    static constexpr ice::String Constant_UIAttribute_StyleTarget = "target";
    static constexpr ice::String Constant_UIAttribute_StyleColor = "color";
    static constexpr ice::String Constant_UIAttribute_StyleTransparency = "alpha";
    static constexpr ice::String Constant_UIAttribute_StyleTargetState = "state";

    static constexpr ice::String Constant_UIResourceType_Font = "font";
    static constexpr ice::String Constant_UIResourceType_Text = "text";
    static constexpr ice::String Constant_UIResourceType_String = "string";
    static constexpr ice::String Constant_UIResourceType_Texture = "texture";

    void parse_styles(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawStyle>& styles
    ) noexcept;

    void parse_resources(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawResource>& resources
    ) noexcept;

    void parse_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawShard>& shards
    ) noexcept;

    void parse_page_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_page,
        ice::Array<RawElement>& elements
    ) noexcept;

} // namespace ice
