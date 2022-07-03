#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_utils.hxx"
#include <ice/ui_data.hxx>
#include <ice/assert.hxx>

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

            ice::RawResource res{ .type = ResourceType::None };
            res.ui_name = ice::xml_value(uiref);

            if (type_str == Constant_UIResourceType_Font)
            {
                rapidxml_ns::xml_attribute<char> const* const fontname_attr = ice::xml_first_attrib(
                    xml_child,
                    ice::Constant_UIAttribute_ResourceFontName
                );
                res.font_data.font_name = ice::xml_value(fontname_attr);

                rapidxml_ns::xml_attribute<char> const* const fontsize_attr = ice::xml_first_attrib(
                    xml_child,
                    ice::Constant_UIAttribute_ResourceFontSize
                );

                ice::Utf8String font_size_str = ice::xml_value(fontsize_attr);
                if (ice::from_chars(font_size_str, font_size_str, res.font_data.font_size))
                {
                    res.type = ResourceType::Font;
                    res.font_data.font_default = res.ui_name == u8"default";
                }
            }
            else if (type_str == Constant_UIResourceType_Text)
            {
                res.type = ResourceType::Utf8String;
            }
            else if (type_str.starts_with(Constant_UIResourceType_String))
            {
                ice::usize const arrval_beg = type_str.find_first_of('[');
                ice::usize const arrval_end = type_str.find_last_of(']');
                if (arrval_beg < arrval_end)
                {
                    ice::Utf8String arrval_str = type_str.substr(arrval_beg + 1, (arrval_end - arrval_beg) - 1);
                    if (ice::from_chars(arrval_str, arrval_str, res.type_data))
                    {
                        res.type = ResourceType::Utf8String;
                    }
                }
            }

            ICE_ASSERT(
                res.type != ResourceType::None,
                "Unknown resource type provided in UI definition!"
            );

            if (uiref && res.type != ResourceType::None)
            {
                ice::pod::array::push_back(
                    shards,
                    res
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
        compile_element(alloc, page, 0, elements);
    }

} // namespace ice
