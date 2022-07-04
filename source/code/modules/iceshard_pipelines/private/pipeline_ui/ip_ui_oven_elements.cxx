#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_utils.hxx"

#include <ice/ui_button.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/assert.hxx>

namespace ice
{

    bool trim(
        ice::Utf8String& inout_str,
        ice::c8utf character
    ) noexcept
    {
        ice::usize const left_bracket = inout_str.find_first_not_of(character);
        ice::usize const right_bracket = inout_str.find_last_not_of(character);
        if (left_bracket == ice::String::npos && right_bracket == ice::String::npos)
        {
            return false;
        }

        inout_str = inout_str.substr(left_bracket, (right_bracket - left_bracket) + 1);
        return true;
    }

    bool remove_brackets(
        ice::Utf8String& inout_str
    ) noexcept
    {
        ice::usize const left_bracket = inout_str.find_first_of('{');
        ice::usize const right_bracket = inout_str.find_last_of('}');
        if (left_bracket == ice::String::npos || right_bracket == ice::String::npos)
        {
            return false;
        }

        inout_str = inout_str.substr(left_bracket + 1, (right_bracket - left_bracket) - 1);
        return true;
    }

    bool parse_action_type(
        ice::Utf8String& inout_str,
        ice::RawAction& out_action
    ) noexcept
    {
        ice::usize const type_end = inout_str.find_first_of(u8' ');

        bool result = false;
        if (type_end != ice::String::npos)
        {
            ice::Utf8String const action_type = inout_str.substr(0, type_end);
            if (action_type == RawAction::Constant_ActionType_Shard)
            {
                out_action.action_type = ice::ui::ActionType::Shard;
            }
            else if (action_type == RawAction::Constant_ActionType_UIShow)
            {
                out_action.action_type = ice::ui::ActionType::UIShow;
            }
            else if (action_type == RawAction::Constant_ActionType_Resource)
            {
                out_action.action_type = ice::ui::ActionType::Data;
            }

            // Result.
            result = out_action.action_type != ice::ui::ActionType::None;
            if (result)
            {
                inout_str = inout_str.substr(type_end + 1);

                ice::usize const action_value_end = inout_str.find_first_of(u8",} ");
                out_action.action_value = inout_str.substr(0, action_value_end);
                result = out_action.action_value.empty() == false;

                // Move to the next expected token.
                inout_str = inout_str.substr(action_value_end + 1);
            }
        }
        return result;
    }

    bool parse_data_reference(
        ice::Utf8String str,
        ice::RawData& out_action
    ) noexcept
    {
        out_action.data_type = ice::ui::DataSource::None;
        out_action.data_source = {};

        bool result = false;
        if (remove_brackets(str))
        {
            ice::usize const type_end = str.find_first_of(u8' ');
            ice::Utf8String const data_type = str.substr(0, type_end);

            if (data_type == RawAction::Constant_ActionDataType_Resource)
            {
                out_action.data_type = ice::ui::DataSource::ValueResource;
                out_action.data_source = str.substr(type_end + 1);
            }
            else if (data_type == RawAction::Constant_ActionDataType_Property)
            {
                out_action.data_type = ice::ui::DataSource::ValueProperty;
                out_action.data_source = str.substr(type_end + 1);
            }
            else if (data_type == RawAction::Constant_ActionDataType_UIPage)
            {
                out_action.data_type = ice::ui::DataSource::ValueUIPage;
                out_action.data_source = str.substr(type_end + 1);
            }

            result = out_action.data_type != ice::ui::DataSource::None;
            if (result)
            {
                ice::trim(out_action.data_source, ' ');
            }
        }

        return result;
    }

    bool parse_action_data(
        ice::Utf8String& inout_str,
        ice::RawAction& out_action
    ) noexcept
    {
        out_action.data.data_type = ice::ui::DataSource::None;
        out_action.data.data_source = {};

        bool result = true;
        ice::usize const data_start = inout_str.find_first_of('=');
        if (data_start != ice::String::npos)
        {
            ice::usize const data_arg_start = inout_str.find_last_of(u8" ,", data_start);
            ice::Utf8String const data_arg = inout_str.substr(data_arg_start + 1, (data_start - data_arg_start) - 1);

            inout_str = inout_str.substr(data_start + 1);
            if (remove_brackets(inout_str))
            {
                ICE_ASSERT(false, "TODO!");
                //ice::usize const type_end = str.find_first_of(u8' ');
                //ice::Utf8String const data_type = str.substr(0, type_end);

                //if (out_action.action_type == ice::ui::ActionType::Shard
                //    && (data_arg == RawAction::Constant_ActionShard_DataArgument)
                //        /*|| data_arg == RawAction::Constant_ActionResource_DataArgument)*/
                //    )
                //{
                //    if (data_type == RawAction::Constant_ActionDataType_Resource)
                //    {
                //        out_action.data_type = ice::ui::DataSource::ValueResource;
                //        out_action.data_source = str.substr(type_end + 1);
                //    }
                //    else if (data_type == RawAction::Constant_ActionDataType_Property)
                //    {
                //        out_action.data_type = ice::ui::DataSource::ValueProperty;
                //        out_action.data_source = str.substr(type_end + 1);
                //    }
                //}
                //else if (out_action.action_type == ice::ui::ActionType::UIShow
                //    && data_arg == RawAction::Constant_ActionUIShow_DataArgument)
                //{
                //    if (data_type == RawAction::Constant_ActionDataType_UIPage)
                //    {
                //        out_action.data_type = ice::ui::DataSource::ValueUIPage;
                //        out_action.data_source = str.substr(type_end + 1);
                //    }
                //}

                //result = out_action.data_type != ice::ui::DataSource::None;
            }
        }
        return result;
    }

    bool parse_action(
        ice::Utf8String action_definition,
        ice::RawAction& out_action
    ) noexcept
    {
        if (ice::remove_brackets(action_definition) == false)
        {
            return false;
        }

        bool result = true;
        if (result = ice::parse_action_type(action_definition, out_action); result)
        {
            result = ice::parse_action_data(action_definition, out_action);
        }
        return result;
    }

    void parse_element_type_button(
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::Allocator& alloc,
        ice::RawElement& info
    ) noexcept
    {
        ice::RawButtonInfo* button_info = alloc.make<RawButtonInfo>();
        button_info->font.data_type = ice::ui::DataSource::Value;
        button_info->font.data_source = u8"default";

        rapidxml_ns::xml_attribute<char> const* attribute = xml_element->first_attribute();
        while (attribute != nullptr)
        {
            ice::String const attrib_name = ice::xml_name(attribute);

            if (attrib_name == ice::Constant_UIAttribute_Text)
            {
                button_info->text = ice::xml_value(attribute);
                if (ice::parse_action(button_info->text, button_info->action_text))
                {
                    button_info->text = {};
                }
            }
            else if (attrib_name == ice::Constant_UIAttribute_Font)
            {
                ice::Utf8String data_ref = ice::xml_value(attribute);
                if (ice::parse_data_reference(data_ref, button_info->font) == false)
                {
                    button_info->font.data_type = ice::ui::DataSource::Value;
                    button_info->font.data_source = data_ref;
                }
            }
            else if (attrib_name == ice::Constant_UIAttribute_OnClick)
            {
                ice::Utf8String const action_value = ice::xml_value(attribute);
                bool const action_parse_result = parse_action(
                    action_value,
                    button_info->action_on_click
                );

                ICE_ASSERT(
                    action_parse_result != false,
                    "Failed to parse action value '{}'",
                    ice::String{ attribute->value(), attribute->value_size() }
                );
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
            parse_element_type_button(xml_element, alloc, info);
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
                info.flags,
                info.size
            );
        }
        else if (info.type == ElementType::VListBox)
        {
            info.flags = info.flags | ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight;
        }

        if (auto const* attr_pos = element->first_attribute("position"); attr_pos != nullptr)
        {
            parse_element_pos(
                attr_pos->value(),
                attr_pos->value() + attr_pos->value_size(),
                info.flags,
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
                info.flags,
                info.margin
            );
        }

        if (auto const* attr_padd = element->first_attribute("padding"); attr_padd != nullptr)
        {
            parse_element_offset(
                attr_padd->value(),
                attr_padd->value() + attr_padd->value_size(),
                info.flags,
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
