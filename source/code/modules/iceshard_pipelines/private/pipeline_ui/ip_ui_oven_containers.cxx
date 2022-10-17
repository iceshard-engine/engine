#include "ip_ui_oven_containers.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_utils.hxx"

#include <ice/ui_element.hxx>

namespace ice
{

    void parse_layout_element(
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::Allocator& alloc,
        ice::RawElement& info
    ) noexcept
    {
        rapidxml_ns::xml_attribute<char> const* type_attrib = ice::xml_first_attrib(xml_element, ice::Constant_UIAttribute_LayoutType);

        ice::String const attrib_value = ice::xml_value(type_attrib);
        if (attrib_value == ice::Constant_UIAttributeKeyword_Vertical)
        {
            info.type = ice::ui::ElementType::LayoutV;
        }
    }

} // namespace ice
