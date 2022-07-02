#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_utils.hxx"
#include <ice/ui_data.hxx>

namespace ice
{

    void compile_resources(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawResource>& shards
    ) noexcept
    {
        using ice::ui::ResourceType;

        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_node,
            Constant_ISUINamespaceUI,
            {}
        );

        while (xml_child != nullptr)
        {
            rapidxml_ns::xml_attribute<char> const* const uiref = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ResourceName
            );

            rapidxml_ns::xml_attribute<char> const* const attr_type = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ResourceType
            );

            ice::Utf8String const type_str = ice::xml_value(attr_type);

            ice::u32 type_data = 0;
            ice::ui::ResourceType type = ResourceType::None;
            if (type_str == Constant_UIResourceType_Text)
            {
                type = ResourceType::Utf8String;
            }
            if (type_str == Constant_UIResourceType_StringShort)
            {
                type = ResourceType::Utf8String;
                type_data = 64;
            }

            if (uiref && type != ResourceType::None)
            {
                ice::pod::array::push_back(
                    shards,
                    ice::RawResource
                    {
                        .ui_name = ice::xml_value(uiref),
                        .type = type,
                        .type_data = type_data,
                    }
                );
            }

            xml_child = ice::xml_next_sibling(xml_child, Constant_ISUINamespaceUI);
        }
    }

    void compile_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawShard>& shards
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_node,
            Constant_ISUINamespaceIceShard,
            {}
        );

        while (xml_child != nullptr)
        {
            rapidxml_ns::xml_attribute<char> const* const uiref = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ShardReference
            );

            rapidxml_ns::xml_attribute<char> const* const attr_name = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ShardName
            );

            if (uiref && attr_name)
            {
                ice::pod::array::push_back(
                    shards,
                    ice::RawShard
                    {
                        .ui_name = ice::xml_value(uiref),
                        .shard_name = ice::shard_name({ attr_name->value(), attr_name->value_size() })
                    }
                );
            }

            xml_child = ice::xml_next_sibling(xml_child, Constant_ISUINamespaceIceShard);
        }
    }

    void compile_page(ice::Allocator& alloc, rapidxml_ns::xml_node<char> const* page, ice::pod::Array<RawElement>& elements) noexcept
    {
        // TODO: Check!
        //ICE_ASSERT(
        //    ice::pod::array::size(elements) < ice::u32{ 1 << 12 },
        //    "Page has more element's than allowed!"
        //);

        compile_element(alloc, page, 0, elements);
    }

} // namespace ice
