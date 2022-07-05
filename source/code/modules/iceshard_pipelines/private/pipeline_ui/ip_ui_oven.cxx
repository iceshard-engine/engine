#include "ip_ui_oven.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_utils.hxx"

namespace ice
{

    void parse_ui_file(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::RawElement>& out_elements,
        ice::pod::Array<ice::RawResource>& out_resources,
        ice::pod::Array<ice::RawShard>& out_shards,
        ice::pod::Array<ice::RawStyle>& out_styles
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* root = ice::xml_first_node(&doc, {}, ice::Constant_UIElement_Root);
        if (root == nullptr)
        {
            return;
        }

        rapidxml_ns::xml_node<char> const* xml_child = ice::xml_first_node(root, ice::Constant_ISUINamespaceUI);
        while (xml_child)
        {
            ice::String const node_name = ice::xml_name(xml_child);
            if (node_name == ice::Constant_UIElementGroup_Resources)
            {
                parse_resources(alloc, xml_child, out_resources);
            }
            else if (node_name == ice::Constant_UIElementGroup_Styles)
            {
                parse_styles(alloc, xml_child, out_styles);
            }
            else if (node_name == ice::Constant_UIElement_Page)
            {
                parse_page_element(alloc, xml_child, out_elements);
            }

            xml_child = ice::xml_next_sibling(xml_child, ice::Constant_ISUINamespaceUI);
        }

        // Special case for shards.
        rapidxml_ns::xml_node<char> const* xml_shards = ice::xml_first_node(
            root,
            ice::Constant_ISUINamespaceIceShard,
            ice::Constant_UIElementGroup_Shards
        );
        if (xml_shards != nullptr)
        {
            parse_shards(alloc, xml_shards, out_shards);
        }
    }

} // namespace ice
