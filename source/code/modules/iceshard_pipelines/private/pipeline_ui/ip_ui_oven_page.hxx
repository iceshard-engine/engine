#pragma once
#include <ice/shard.hxx>
#include <ice/pod/array.hxx>
#include <ice/ui_data.hxx>

#include "ip_ui_oven.hxx"

namespace ice
{

    static constexpr ice::String Constant_UIElementGroup_Resources = "resources";
    static constexpr ice::String Constant_UIElementGroup_Shards = "shards";

    static constexpr ice::String Constant_UIElement_Resource = "resource";
    static constexpr ice::String Constant_UIElement_Shard = "shard";
    static constexpr ice::String Constant_UIElement_Page = "page";

    static constexpr ice::String Constant_UIAttribute_ResourceType = "type";
    static constexpr ice::String Constant_UIAttribute_ResourceName = "name";
    static constexpr ice::String Constant_UIAttribute_ResourceFontSize = "size";
    static constexpr ice::String Constant_UIAttribute_ResourceFontName = "font";

    static constexpr ice::String Constant_UIAttribute_ShardReference = "name";
    static constexpr ice::String Constant_UIAttribute_ShardName = "action";

    static constexpr ice::Utf8String Constant_UIResourceType_Font = u8"font";
    static constexpr ice::Utf8String Constant_UIResourceType_Text = u8"text";
    static constexpr ice::Utf8String Constant_UIResourceType_String = u8"string";

    struct RawElement
    {
        ice::u16 parent;
        ice::ui::Size size;
        ice::ui::Position position;
        ice::ui::RectOffset margin;
        ice::ui::RectOffset padding;

        ice::ui::ElementFlags size_flags;
        ice::ui::ElementFlags position_flags;
        ice::ui::ElementFlags margin_flags;
        ice::ui::ElementFlags padding_flags;

        ice::ui::ElementType type;
        void* type_data;
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

    void compile_resources(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawResource>& resources
    ) noexcept;

    void compile_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawShard>& shards
    ) noexcept;

    void compile_page(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* page,
        ice::pod::Array<RawElement>& elements
    ) noexcept;

} // namespace ice
