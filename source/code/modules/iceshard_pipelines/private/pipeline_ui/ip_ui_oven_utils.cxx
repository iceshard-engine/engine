#include "ip_ui_oven_utils.hxx"
#include <ice/ui_element_info.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto xml_first_node(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_node<char> const*
    {
        if (name.empty())
        {
            return parent->first_node_ns(
                node_ns.data(),
                node_ns.size()
            );
        }
        else
        {
            return parent->first_node_ns(
                node_ns.data(),
                node_ns.size(),
                name.data(),
                name.size()
            );
        }
    }

    auto xml_next_sibling(
        rapidxml_ns::xml_node<char> const* parent,
        ice::String node_ns,
        ice::String name
    ) noexcept -> rapidxml_ns::xml_node<char> const*
    {
        if (name.empty())
        {
            return parent->next_sibling_ns(
                node_ns.data(),
                node_ns.size()
            );
        }
        else
        {
            return parent->next_sibling_ns(
                node_ns.data(),
                node_ns.size(),
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

    void parse_element_size(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::Size& size) noexcept
    {
        using ice::ui::ElementFlags;

        ice::String attr_size{ it, end };
        ice::usize const separator = attr_size.find_first_of(',');

        bool valid_values = true;
        if (separator != ice::String::npos)
        {
            ice::String left = attr_size.substr(0, separator);
            ice::String right = attr_size.substr(separator + 1);

            if (ice::from_chars(left, left, size.width) == false)
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
            if (ice::from_chars(right, right, size.width) == false)
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
        else if (ice::from_chars(attr_size, attr_size, size.width))
        {
            size.height = size.width;
        }
        else if (attr_size == ice::Constant_UIAttributeKeyword_Auto)
        {
            out_flags = out_flags | ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight;
        }
        else if (attr_size == ice::Constant_UIAttributeKeyword_Stretch)
        {
            out_flags = out_flags | ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight;
        }
        else
        {
            valid_values = false;
        }

        ICE_ASSERT(
            valid_values || attr_size.empty(),
            "Invalid value in 'size' attribute! Valid values are: {}, {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto,
            ice::Constant_UIAttributeKeyword_Stretch
        );
    }

    void parse_element_pos(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::Position& pos) noexcept
    {
        using ice::ui::ElementFlags;

        ice::String attr_position{ it, end };
        ice::usize const separator = attr_position.find_first_of(',');

        bool valid_values = true;
        if (separator != ice::String::npos)
        {
            ice::String left = attr_position.substr(0, separator);
            ice::String right = attr_position.substr(separator + 1);

            if (ice::from_chars(left, left, pos.x) == false)
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
            if (ice::from_chars(right, right, pos.y) == false)
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
        }
        else if (ice::from_chars(attr_position, attr_position, pos.x))
        {
            pos.y = pos.x;
        }
        else if (attr_position == ice::Constant_UIAttributeKeyword_Auto)
        {
            out_flags = out_flags | ElementFlags::Position_AutoX | ElementFlags::Position_AutoY;
        }
        else
        {
            valid_values = false;
        }

        ICE_ASSERT(
            valid_values || attr_position.empty(),
            "Invalid value in 'position' attribute! Valid values are: {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto
        );
    }

    void parse_element_offset(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::RectOffset& offset) noexcept
    {
        using ice::ui::ElementFlags;

        ice::String attr_offset{ it, end };
        ice::usize const sep1 = attr_offset.find_first_of(',', 0);
        ice::usize const sep2 = attr_offset.find_first_of(',', sep1 + 1);
        ice::usize const sep3 = attr_offset.find_first_of(',', sep2 + 1);

        bool valid_values = true;
        if (sep3 != ice::String::npos && sep2 != ice::String::npos && sep1 != ice::string_npos)
        {
            ice::String first = attr_offset.substr(0, sep1);
            ice::String second = attr_offset.substr(sep1 + 1, (sep2 - sep1) - 1);
            ice::String third = attr_offset.substr(sep2 + 1, (sep3 - sep2) - 1);
            ice::String fourth = attr_offset.substr(sep3 + 1);

            if (ice::from_chars(first, first, offset.left) == false)
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
            if (ice::from_chars(second, second, offset.top) == false)
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
            if (ice::from_chars(third, third, offset.right) == false)
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
            if (ice::from_chars(fourth, fourth, offset.bottom) == false)
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
            ice::String first = attr_offset.substr(0, sep1);
            ice::String second = attr_offset.substr(sep1 + 1);

            if (ice::from_chars(first, first, offset.left) == false)
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
                offset.right = offset.left;
            }

            if (ice::from_chars(second, second, offset.top) == false)
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
                offset.bottom = offset.top;
            }
        }
        else if (ice::from_chars(attr_offset, attr_offset, offset.left))
        {
            offset.top = offset.left;
            offset.right = offset.left;
            offset.bottom = offset.left;
        }
        else if (attr_offset == ice::Constant_UIAttributeKeyword_Auto)
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
            valid_values || attr_offset.empty(),
            "Invalid value in 'padding' / 'margin' attribute! Valid values are: {}, <float>.",
            ice::Constant_UIAttributeKeyword_Auto
        );
    }
} // namespace ice
