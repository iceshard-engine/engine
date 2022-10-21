#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_containers.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_utils.hxx"
#include <ice/ui_resource.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/string_utils.hxx>
#include <ice/assert.hxx>

namespace ice
{

    void parse_style_entry(
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::RawStyle& style
    ) noexcept
    {
        ice::String const node_name = ice::xml_name(xml_node);
        if (node_name == ice::Constant_UIElement_StyleBackgroud)
        {
            rapidxml_ns::xml_attribute<char> const* attrib = ice::xml_first_attrib(xml_node, {});
            while (attrib)
            {
                ice::String const attrib_name = ice::xml_name(attrib);

                if (attrib_name == ice::Constant_UIAttribute_StyleColor)
                {
                    style.flags |= ice::ui::StyleFlags::BackgroundColor;
                    ice::parse_element_color(xml_value_noutf8(attrib), style.background.color);
                }
                else if (attrib_name == ice::Constant_UIAttribute_StyleTransparency)
                {
                    style.flags |= ice::ui::StyleFlags::BackgroundColor;
                    ice::from_chars(ice::xml_value(attrib), style.background.color.alpha);
                }

                attrib = ice::xml_next_attrib(attrib, {});
            }
        }
    }

    auto parse_element_states(ice::String element_state) noexcept -> ice::ui::ElementState
    {
        using ice::ui::ElementState;

        ElementState result = ElementState::None;
        if (ice::string::empty(element_state))
        {
            result |= ElementState::Any;
        }
        else
        {
            if (element_state == "hoover")
            {
                result |= ElementState::Hoover;
            }

            ICE_ASSERT(result != ElementState::None, "Invalid value for 'state' attribute in 'style' element.");
        }
        return result;
    }

    auto parse_element_type(ice::String element_name) noexcept -> ice::ui::ElementType
    {
        using ice::ui::ElementType;
        if (element_name == ice::Constant_UIElement_Page)
        {
            return ElementType::Page;
        }
        else if (element_name == ice::Constant_UIElement_Button)
        {
            return ElementType::Button;
        }
        else if (element_name == ice::Constant_UIElement_Label)
        {
            return ElementType::Label;
        }
        return ElementType::Any;
    }

    void parse_styles(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawStyle>& styles
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_node,
            ice::Constant_ISUINamespaceUI,
            ice::Constant_UIElement_Style
        );

        while (xml_child != nullptr)
        {
            rapidxml_ns::xml_attribute<char> const* const attr_name = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_StyleName
            );
            if (attr_name)
            {
                ice::RawStyle style{ };
                style.name = ice::xml_value(attr_name);
                style.target_element = ice::ui::ElementType::Any;

                ice::String const target_element = ice::xml_value_noutf8(
                    ice::xml_first_attrib(xml_child, ice::Constant_UIAttribute_StyleTarget)
                );
                style.target_element = parse_element_type(target_element);

                ice::String const target_state = ice::xml_value_noutf8(
                    ice::xml_first_attrib(xml_child, ice::Constant_UIAttribute_StyleTargetState)
                );
                style.target_element_states = parse_element_states(target_state);

                rapidxml_ns::xml_node<char> const* xml_prop = ice::xml_first_node(xml_child);
                while(xml_prop != nullptr)
                {
                    parse_style_entry(xml_prop, style);
                    xml_prop = ice::xml_next_sibling(xml_prop);
                }

                ice::array::push_back(styles, style);
            }

            xml_child = ice::xml_next_sibling(
                xml_child,
                ice::Constant_ISUINamespaceUI,
                ice::Constant_UIElement_Style
            );
        }
    }

    void parse_resources(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawResource>& shards
    ) noexcept
    {
        using ice::ui::ResourceType;

        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_node,
            ice::Constant_ISUINamespaceUI,
            ice::Constant_UIElement_Resource
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

            ice::String const type_str = ice::xml_value(attr_type);

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

                if (ice::from_chars(ice::xml_value(fontsize_attr), res.font_data.font_size))
                {
                    res.type = ResourceType::Font;
                    res.font_data.font_default = res.ui_name == "default";
                }
            }
            else if (type_str == Constant_UIResourceType_Text)
            {
                res.type = ResourceType::String;
            }
            else if (ice::string::starts_with(type_str, Constant_UIResourceType_String))
            {
                ice::ucount const arrval_beg = ice::string::find_first_of(type_str, '[');
                ice::ucount const arrval_end = ice::string::find_last_of(type_str, ']');
                if (arrval_beg < arrval_end)
                {
                    ice::String arrval_str = ice::string::substr(type_str, arrval_beg + 1, (arrval_end - arrval_beg) - 1);
                    if (ice::from_chars(arrval_str, res.type_data))
                    {
                        res.type = ResourceType::String;
                    }
                }
            }
            else if (type_str == Constant_UIResourceType_Texture)
            {
                res.type = ResourceType::Texture;
            }

            ICE_ASSERT(
                res.type != ResourceType::None,
                "Unknown resource type provided in UI definition!"
            );

            if (uiref && res.type != ResourceType::None)
            {
                ice::array::push_back(
                    shards,
                    res
                );
            }

            xml_child = ice::xml_next_sibling(
                xml_child,
                ice::Constant_ISUINamespaceUI,
                ice::Constant_UIElement_Resource
            );
        }
    }

    void parse_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::Array<ice::RawShard>& shards
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_node,
            ice::Constant_ISUINamespaceIceShard,
            ice::Constant_UIElement_Shard
        );

        while (xml_child != nullptr)
        {
            rapidxml_ns::xml_attribute<char> const* const uiref = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ShardName
            );

            rapidxml_ns::xml_attribute<char> const* const attr_name = ice::xml_first_attrib(
                xml_child,
                ice::Constant_UIAttribute_ShardAction
            );

            if (uiref && attr_name)
            {
                ice::array::push_back(
                    shards,
                    ice::RawShard
                    {
                        .ui_name = ice::xml_value(uiref),
                        .shard_name = ice::shardid(ice::String{ attr_name->value(), ice::ucount(attr_name->value_size()) })
                    }
                );
            }

            xml_child = ice::xml_next_sibling(
                xml_child,
                ice::Constant_ISUINamespaceIceShard,
                ice::Constant_UIElement_Shard
            );
        }
    }

    void parse_child_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::u16 parent_idx,
        ice::Array<RawElement>& elements
    ) noexcept
    {
        ice::u16 const element_index = static_cast<ice::u16>(
            ice::array::count(elements)
        );

        ice::array::push_back(
            elements,
            ice::RawElement{ .parent = parent_idx }
        );

        parse_element_attribs(
            alloc,
            xml_element,
            elements[element_index]
        );

        parse_element_details(
            alloc,
            xml_element,
            elements[element_index]
        );

        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(
            xml_element,
            ice::Constant_ISUINamespaceUI
        );

        while (xml_child != nullptr)
        {
            parse_child_element(alloc, xml_child, element_index, elements);

            xml_child = ice::xml_next_sibling(xml_child, ice::Constant_ISUINamespaceUI);
        }
    }

    void parse_page_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_page,
        ice::Array<RawElement>& elements
    ) noexcept
    {
        parse_child_element(alloc, xml_page, 0, elements);
    }

} // namespace ice
