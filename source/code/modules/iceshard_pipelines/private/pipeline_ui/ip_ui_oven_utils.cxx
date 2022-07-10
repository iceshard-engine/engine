#include "ip_ui_oven_utils.hxx"
#include <ice/ui_element_info.hxx>
#include <ice/ui_style.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto xml_first_node(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String ns,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_node<char> const*
    {
        if (!ns.empty())
        {
            if (!name.empty())
            {
                return parent->first_node_ns(
                    ns.data(),
                    ns.size(),
                    name.data(),
                    name.size()
                );
            }
            else
            {
                return parent->first_node_ns(
                    ns.data(),
                    ns.size()
                );
            }
        }
        else
        {
            return parent->first_node(
                name.data(),
                name.size()
            );
        }
    }

    auto xml_next_sibling(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String ns,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_node<char> const*
    {
        if (!ns.empty())
        {
            if (!name.empty())
            {
                return parent->next_sibling_ns(
                    ns.data(),
                    ns.size(),
                    name.data(),
                    name.size()
                );
            }
            else
            {
                return parent->next_sibling_ns(
                    ns.data(),
                    ns.size()
                );
            }
        }
        else
        {
            return parent->next_sibling(
                name.data(),
                name.size()
            );
        }
    }

    auto xml_first_attrib(
        rapidxml_ns::xml_node<char> const* node,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_attribute<char> const*
    {
        return node->first_attribute(
            name.data(),
            name.size()
        );
    }

    auto xml_next_attrib(
        rapidxml_ns::xml_attribute<char> const* attrib,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_attribute<char> const*
    {
        return attrib->next_attribute(
            name.data(),
            name.size()
        );
    }

    auto xml_name(
        rapidxml_ns::xml_node<char> const* node
    ) noexcept -> ice::String
    {
        return { node->local_name(), node->local_name_size() };
    }

    auto xml_name(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::String
    {
        return { attrib->name(), attrib->name_size() };
    }

    auto xml_value(
        rapidxml_ns::xml_node<char> const* node
    ) noexcept -> ice::Utf8String
    {
        return { reinterpret_cast<ice::c8utf const*>(node->value()), node->value_size() };
    }

    auto xml_value(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::Utf8String
    {
        return attrib == nullptr ? u8"" : ice::Utf8String{ reinterpret_cast<ice::c8utf const*>(attrib->value()), attrib->value_size() };
    }

    auto xml_value_noutf8(
        rapidxml_ns::xml_attribute<char> const* attrib
    ) noexcept -> ice::String
    {
        return attrib == nullptr ? "" : ice::String{ attrib->value(), attrib->value_size() };
    }

    void parse_element_size(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Size& out_size
    ) noexcept
    {
        using ice::ui::ElementFlags;

        ice::usize const separator = value.find_first_of(',');

        bool valid_values = true;
        if (separator != ice::String::npos)
        {
            ice::String left = value.substr(0, separator);
            ice::String right = value.substr(separator + 1);

            if (ice::from_chars(left, left, out_size.width) == false)
            {
                if (left == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoWidth;
                }
                else if (left == ice::Constant_UIAttributeKeyword_Stretch)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchWidth;
                }
                else
                {
                    valid_values = false;
                }
            }
            if (ice::from_chars(right, right, out_size.height) == false)
            {
                if (right == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoHeight;
                }
                else if (right == ice::Constant_UIAttributeKeyword_Stretch)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchHeight;
                }
                else
                {
                    valid_values = false;
                }
            }
        }
        else if (ice::from_chars(value, value, out_size.width))
        {
            out_size.height = out_size.width;
        }
        else if (value == ice::Constant_UIAttributeKeyword_Auto)
        {
            out_flags = out_flags | ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight;
        }
        else if (value == ice::Constant_UIAttributeKeyword_Stretch)
        {
            out_flags = out_flags | ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight;
        }
        else
        {
            valid_values = false;
        }

        ICE_ASSERT(
            valid_values || value.empty(),
            "Invalid value in 'size' attribute! Valid values are: {}, {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto,
            ice::Constant_UIAttributeKeyword_Stretch
        );
    }

    void parse_element_pos(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Position& out_pos
    ) noexcept
    {
        using ice::ui::ElementFlags;

        ice::usize const separator = value.find_first_of(',');

        bool valid_values = true;
        if (separator != ice::String::npos)
        {
            ice::String left = value.substr(0, separator);
            ice::String right = value.substr(separator + 1);

            if (ice::from_chars(left, left, out_pos.x) == false)
            {
                if (left == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoX;
                }
                else
                {
                    valid_values = false;
                }
            }
            else if (left == "%")
            {
                out_flags = out_flags | ElementFlags::Position_PercentageX;
            }
            if (ice::from_chars(right, right, out_pos.y) == false)
            {
                if (right == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoY;
                }
                else
                {
                    valid_values = false;
                }
            }
            else if (right == "%")
            {
                out_flags = out_flags | ElementFlags::Position_PercentageY;
            }
        }
        else if (ice::from_chars(value, value, out_pos.x))
        {
            out_pos.y = out_pos.x;
        }
        else if (value == ice::Constant_UIAttributeKeyword_Auto)
        {
            out_flags = out_flags | ElementFlags::Position_AutoX | ElementFlags::Position_AutoY;
        }
        else
        {
            valid_values = false;
        }

        ICE_ASSERT(
            valid_values || value.empty(),
            "Invalid value in 'position' attribute! Valid values are: {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto
        );
    }

    void parse_element_offset(
        ice::String value,
        ice::ui::ElementFlags& out_flags,
        ice::ui::RectOffset& out_offset
    ) noexcept
    {
        using ice::ui::ElementFlags;

        ice::usize const sep1 = value.find_first_of(',', 0);
        ice::usize const sep2 = value.find_first_of(',', sep1 + 1);
        ice::usize const sep3 = value.find_first_of(',', sep2 + 1);

        bool valid_values = true;
        if (sep3 != ice::String::npos && sep2 != ice::String::npos && sep1 != ice::string_npos)
        {
            ice::String first = value.substr(0, sep1);
            ice::String second = value.substr(sep1 + 1, (sep2 - sep1) - 1);
            ice::String third = value.substr(sep2 + 1, (sep3 - sep2) - 1);
            ice::String fourth = value.substr(sep3 + 1);

            if (ice::from_chars(first, first, out_offset.left) == false)
            {
                if (first == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoLeft;
                }
                else
                {
                    valid_values = false;
                }
            }
            if (ice::from_chars(second, second, out_offset.top) == false)
            {
                if (second == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoTop;
                }
                else
                {
                    valid_values = false;
                }
            }
            if (ice::from_chars(third, third, out_offset.right) == false)
            {
                if (third == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoRight;
                }
                else
                {
                    valid_values = false;
                }
            }
            if (ice::from_chars(fourth, fourth, out_offset.bottom) == false)
            {
                if (fourth == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoBottom;
                }
                else
                {
                    valid_values = false;
                }
            }
        }
        else if (sep1 != ice::String::npos)
        {
            ice::String first = value.substr(0, sep1);
            ice::String second = value.substr(sep1 + 1);

            if (ice::from_chars(first, first, out_offset.left) == false)
            {
                if (first == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoLeft | ElementFlags::Offset_AutoRight;
                }
                else
                {
                    valid_values = false;
                }
            }
            else
            {
                out_offset.right = out_offset.left;
            }

            if (ice::from_chars(second, second, out_offset.top) == false)
            {
                if (second == ice::Constant_UIAttributeKeyword_Auto)
                {
                    out_flags = out_flags | ElementFlags::Offset_AutoTop | ElementFlags::Offset_AutoBottom;
                }
                else
                {
                    valid_values = false;
                }
            }
            else
            {
                out_offset.bottom = out_offset.top;
            }
        }
        else if (ice::from_chars(value, value, out_offset.left))
        {
            out_offset.top = out_offset.left;
            out_offset.right = out_offset.left;
            out_offset.bottom = out_offset.left;
        }
        else if (value == ice::Constant_UIAttributeKeyword_Auto)
        {
            out_flags = out_flags
                | ElementFlags::Offset_AutoLeft
                | ElementFlags::Offset_AutoRight
                | ElementFlags::Offset_AutoTop
                | ElementFlags::Offset_AutoBottom;
        }
        else
        {
            valid_values = false;
        }

        ICE_ASSERT(
            valid_values || value.empty(),
            "Invalid value in 'padding' / 'margin' attribute! Valid values are: {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto
        );
    }

    void parse_element_color(
        ice::String value,
        ice::ui::StyleColor& out_color
    ) noexcept
    {
        using ice::ui::ElementFlags;

        ice::usize const sep1 = value.find_first_of(',', 0);
        ice::usize const sep2 = value.find_first_of(',', sep1 + 1);

        bool valid_values = false;
        if (sep1 != ice::String::npos && sep2 != ice::String::npos)
        {
            ice::String first = value.substr(0, sep1);
            ice::String second = value.substr(sep1 + 1, (sep2 - sep1) - 1);
            ice::String third = value.substr(sep2 + 1);

            ice::from_chars(first, first, out_color.red);
            ice::from_chars(second, second, out_color.green);
            ice::from_chars(third, third, out_color.blue);
            valid_values = true;
        }
        else if (sep1 == ice::String::npos && sep2 == ice::String::npos)
        {
            ice::from_chars(value, value, out_color.red);
            out_color.green = out_color.red;
            out_color.blue = out_color.red;
            valid_values = true;
        }

        ICE_ASSERT(
            valid_values,
            "Invalid value in 'color' attribute! Valid values are: <float>"
        );
    }

} // namespace ice
