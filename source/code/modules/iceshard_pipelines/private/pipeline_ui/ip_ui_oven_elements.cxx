#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_utils.hxx"

#include <ice/ui_button.hxx>
#include <ice/ui_element_info.hxx>

namespace ice
{

    void compile_element_type(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::RawElement& info
    ) noexcept
    {
        info.type = ElementType::Page;
        info.type_data = nullptr;

        if (strcmp(xml_element->local_name(), "container") == 0)
        {
            rapidxml_ns::xml_attribute<char> const* const attrib = xml_element->first_attribute("type");
            if (attrib != nullptr && strcmp(attrib->value(), "vertical") == 0)
            {
                info.type = ElementType::VListBox;
            }
        }
        else if (strcmp(xml_element->local_name(), "button") == 0)
        {
            ice::RawButtonInfo* button_info = reinterpret_cast<ice::RawButtonInfo*>(alloc.allocate(sizeof(RawButtonInfo)));

            rapidxml_ns::xml_attribute<char> const* attribute = xml_element->first_attribute();
            while (attribute != nullptr)
            {
                button_info->action_on_click.type_id = ice::ui::ActionType::None;

                if (strcmp(attribute->name(), "text") == 0)
                {
                    button_info->text = std::u8string_view{ reinterpret_cast<ice::c8utf const*>(attribute->value()), attribute->value_size() };
                }
                else if (strcmp(attribute->name(), "on_click") == 0)
                {
                    ice::Utf8String on_click_action{ (ice::c8utf const*)attribute->value(), attribute->value_size() };
                    on_click_action = on_click_action.substr(1, on_click_action.size() - 2);

                    ice::Utf8String const action_type = on_click_action.substr(0, on_click_action.find_first_of(u8' '));
                    ice::usize const action_type_offset = action_type.length() + 1;
                    ice::usize const action_type_end = on_click_action.find_first_of(u8",}", action_type_offset);
                    ice::Utf8String const type_value = on_click_action.substr(
                        action_type_offset,
                        action_type_end - action_type_offset
                    );

                    button_info->action_on_click.type_id = action_type == u8"Shard"
                        ? ice::ui::ActionType::Shard
                        : ice::ui::ActionType::UIShow;
                    button_info->action_on_click.action_id = type_value;

                    if (on_click_action[action_type_end] == ',')
                    {
                        on_click_action = on_click_action.substr(action_type_end + 1);
                        if (on_click_action.starts_with(u8' '))
                        {
                            on_click_action.remove_prefix(on_click_action.find_first_not_of(u8' '));
                        }

                        ice::Utf8String const argument_name = on_click_action.substr(0, on_click_action.find_first_of(u8'='));
                        if (argument_name == u8"value" && action_type == u8"Shard")
                        {
                            on_click_action = on_click_action.substr(on_click_action.find_first_of('{') + 1);
                            on_click_action.remove_suffix(1);

                            ice::usize const split_offset = on_click_action.find_first_of(u8' ');
                            ice::Utf8String const value_type = on_click_action.substr(0, split_offset);
                            ice::Utf8String const value_source = on_click_action.substr(split_offset + 1);

                            button_info->action_on_click.type_value = value_type == u8"Property"
                                ? ice::ui::ActionData::ValueProperty
                                : ice::ui::ActionData::ValueUIPage;
                            button_info->action_on_click.action_value = value_source;
                        }
                    }
                }

                attribute = attribute->next_attribute();
            }

            if (auto const* xml_node = xml_element->first_node("text"))
            {
                button_info->text = std::u8string_view{ reinterpret_cast<ice::c8utf const*>(xml_node->value()), xml_node->value_size() };
            }

            info.type_data = button_info;
            info.type = ice::ui::ElementType::Button;
        }
    }

    void compile_element_attribs(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* element,
        ice::RawElement& info
    ) noexcept
    {
        using ice::ui::ElementFlags;
        compile_element_type(alloc, element, info);

        if (auto const* attr_size = element->first_attribute("size"); attr_size != nullptr)
        {
            parse_element_size(
                attr_size->value(),
                attr_size->value() + attr_size->value_size(),
                info.size_flags,
                info.size
            );
        }
        else if (info.type == ElementType::Page)
        {
            info.size_flags = ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight;
        }
        else if (info.type == ElementType::VListBox)
        {
            info.size_flags = ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight;
        }

        if (auto const* attr_pos = element->first_attribute("position"); attr_pos != nullptr)
        {
            parse_element_pos(
                attr_pos->value(),
                attr_pos->value() + attr_pos->value_size(),
                info.position_flags,
                info.position
            );
        }

        //if (auto const* attr_anchor = xml_element->first_attribute("anchor"); attr_anchor != nullptr)
        //{
        //    if (strcmp(attr_anchor->value(), "left") == 0)
        //    {
        //        info.position_flags = info.position_flags | ElementFlags::Position_AnchorLeft;
        //    }
        //    else if (strcmp(attr_anchor->value(), "top") == 0)
        //    {
        //        info.position_flags = info.position_flags | ElementFlags::Position_AnchorTop;
        //    }
        //    else if (strcmp(attr_anchor->value(), "right") == 0)
        //    {
        //        info.position_flags = info.position_flags | ElementFlags::Position_AnchorRight;
        //    }
        //    else if (strcmp(attr_anchor->value(), "bottom") == 0)
        //    {
        //        info.position_flags = info.position_flags | ElementFlags::Position_AnchorBottom;
        //    }
        //}
        //else
        //{
        //    info.position_flags = info.position_flags | ElementFlags::Position_AnchorLeft;
        //}

        if (auto const* attr_marg = element->first_attribute("margin"); attr_marg != nullptr)
        {
            parse_element_offset(
                attr_marg->value(),
                attr_marg->value() + attr_marg->value_size(),
                info.margin_flags,
                info.margin
            );
        }

        if (auto const* attr_padd = element->first_attribute("padding"); attr_padd != nullptr)
        {
            parse_element_offset(
                attr_padd->value(),
                attr_padd->value() + attr_padd->value_size(),
                info.padding_flags,
                info.padding
            );
        }
    }

    void compile_element(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::u16 parent_idx,
        ice::pod::Array<RawElement>& elements
    ) noexcept
    {
        ice::u16 const element_index = static_cast<ice::u16>(
            ice::pod::array::size(elements)
            );

        ice::pod::array::push_back(
            elements,
            ice::RawElement{ .parent = parent_idx }
        );

        compile_element_attribs(
            alloc,
            xml_element,
            elements[element_index]
        );

        rapidxml_ns::xml_node<char> const* xml_child = xml_element->first_node_ns(
            Constant_ISUINamespaceUI.data(),
            Constant_ISUINamespaceUI.size()
        );

        while (xml_child != nullptr)
        {
            compile_element(alloc, xml_child, element_index, elements);
            xml_child = xml_child->next_sibling_ns(
                Constant_ISUINamespaceUI.data(),
                Constant_ISUINamespaceUI.size()
            );
        }
    }

} // namespace ice
