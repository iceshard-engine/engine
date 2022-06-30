#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"

namespace ice
{

    void compile_shards(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_node,
        ice::pod::Array<ice::RawShard>& shards
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* xml_child = xml_node->first_node_ns(
            Constant_ISUINamespaceIceShard.data(),
            Constant_ISUINamespaceIceShard.size()
        );

        while (xml_child != nullptr)
        {
            rapidxml_ns::xml_attribute<char> const* const attr_name = xml_child->first_attribute("name");
            rapidxml_ns::xml_attribute<char> const* const attr_action = xml_child->first_attribute("action");

            if (attr_name && attr_action)
            {
                ice::pod::array::push_back(
                    shards,
                    ice::RawShard
                    {
                        .ui_name = ice::Utf8String{ (ice::c8utf const*)attr_name->value(), attr_name->value_size() },
                        .shard_name = ice::shard_name({ attr_action->value(), attr_action->value_size() })
                    }
                );
            }

            xml_child = xml_child->next_sibling_ns(
                Constant_ISUINamespaceIceShard.data(),
                Constant_ISUINamespaceIceShard.size()
            );
        }
    }

    void compile_page(ice::Allocator& alloc, rapidxml_ns::xml_node<char> const* page, ice::pod::Array<RawElement>& elements) noexcept
    {
        // TODO: Check!
        //ICE_ASSERT(
        //    ice::pod::array::size(elements) < ice::u32{ 1 << 12 },
        //    "Page has more element's than allowed!"
        //);

        compile_element(alloc, page, 0, elements);
    }

} // namespace ice
