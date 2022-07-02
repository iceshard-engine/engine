#include "ip_ui_oven.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"
#include "ip_ui_oven_utils.hxx"

namespace ice
{

    void compile_ui(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::RawElement>& raw_elements,
        ice::pod::Array<ice::RawResource>& ui_resources,
        ice::pod::Array<ice::RawShard>& ui_shards
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* root = doc.first_node("isui");
        if (root == nullptr)
        {
            return;
        }

        rapidxml_ns::xml_node<char> const* xml_node_resources = xml_first_node(
            root,
            Constant_ISUINamespaceUI,
            ice::Constant_UIElementGroup_Resources
        );
        if (xml_node_resources != nullptr)
        {
            compile_resources(alloc, xml_node_resources, ui_resources);
        }

        rapidxml_ns::xml_node<char> const* xml_node_shards = xml_first_node(
            root,
            ice::Constant_ISUINamespaceIceShard,
            ice::Constant_UIElementGroup_Shards
        );
        if (xml_node_shards != nullptr)
        {
            compile_shards(alloc, xml_node_shards, ui_shards);
        }

        rapidxml_ns::xml_node<char> const* xml_node = xml_first_node(
            root,
            ice::Constant_ISUINamespaceUI,
            ice::Constant_UIElement_Page
        );

        if (xml_node != nullptr)
        {
            compile_page(alloc, xml_node, raw_elements);
        }

    }

} // namespace ice
