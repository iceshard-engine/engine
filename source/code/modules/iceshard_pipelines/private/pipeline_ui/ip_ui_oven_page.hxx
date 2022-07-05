#pragma once
#include <ice/shard.hxx>
#include <ice/pod/array.hxx>
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

    static constexpr ice::Utf8String Constant_UIResourceType_Font = u8"font";
    static constexpr ice::Utf8String Constant_UIResourceType_Text = u8"text";
    static constexpr ice::Utf8String Constant_UIResourceType_String = u8"string";
    static constexpr ice::Utf8String Constant_UIResourceType_Texture = u8"texture";

    struct RawElement
    {
        ice::u16 parent;
        ice::ui::Size size;
        ice::ui::Position position;
        ice::ui::RectOffset margin;
        ice::ui::RectOffset padding;

        ice::ui::ElementFlags flags;
        ice::ui::ElementType type;
        void* type_data;
    };

    struct RawStyle
    {
        ice::Utf8String name;
        ice::Utf8String target_element;

        ice::ui::StyleFlags flags;

        union StyleData
        {
            struct
            {
                ice::Utf8String name;
                ice::vec2f uvs[4];
            } texture;
            ice::ui::StyleColor color;
        };

        StyleData background;
    };

    struct RawResource
    {
        ice::Utf8String ui_name;
        ice::ui::ResourceType type;

        struct FontData
        {
            ice::Utf8String font_name;
            ice::u16 font_size;
            ice::u16 font_default : 1;
        };

        union
        {
            ice::u32 type_data;
            FontData font_data;
        };
    };

    struct RawShard
    {
        ice::Utf8String ui_name;
        ice::ShardName shard_name;
    };

    void parse_styles(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawStyle>& styles
    ) noexcept;

    void parse_resources(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawResource>& resources
    ) noexcept;

    void parse_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawShard>& shards
    ) noexcept;

    void parse_page_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_page,
        ice::pod::Array<RawElement>& elements
    ) noexcept;

} // namespace ice
