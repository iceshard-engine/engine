#include "ip_ui_oven.hxx"
#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"

namespace ice
{

    void compile_ui(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::RawElement>& raw_elements,
        ice::pod::Array<ice::RawShard>& ui_shards
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* root = doc.first_node("isui");
        if (root == nullptr)
        {
            return;
        }

        rapidxml_ns::xml_node<char> const* xml_node_shards = root->first_node_ns(
            Constant_ISUINamespaceIceShard.data(),
            Constant_ISUINamespaceIceShard.size(),
            "shards",
            6
        );

        if (xml_node_shards != nullptr)
        {
            compile_shards(alloc, xml_node_shards, ui_shards);
        }

        rapidxml_ns::xml_node<char> const* xml_node = root->first_node_ns(
            Constant_ISUINamespaceUI.data(),
            Constant_ISUINamespaceUI.size(),
            "page",
            4
        );

        if (xml_node != nullptr)
        {
            compile_page(alloc, xml_node, raw_elements);
        }

    }

} // namespace ice
