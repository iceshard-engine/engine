#include <ice/ui_asset.hxx>
#include <ice/ui_data.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>

#include <ice/assert.hxx>
#include <ice/log.hxx>

#include <rapidxml_ns/rapidxml_ns.hpp>
#undef assert

namespace ice::ui
{

    static constexpr ice::String Constant_ISUINamespaceUI = "https://www.iceshard.net/docs/engine/v1_alpha/isui/ui/";
    static constexpr ice::String Constant_ISUINamespaceIceShard = "https://www.iceshard.net/docs/engine/v1_alpha/isui/iceshard/";

    struct RawElement
    {
        ice::u16 parent;
        ice::ui::Size size;
        ice::ui::Position position;
        ice::ui::RectOffset margin;
        ice::ui::RectOffset padding;

        ice::ui::ElementFlags size_flags;
        ice::ui::ElementFlags position_flags;
        ice::ui::ElementFlags margin_flags;
        ice::ui::ElementFlags padding_flags;

        ice::ui::ElementType type;
        void* type_data;
    };

    void compile_ui(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::ui::RawElement>& raw_elements
    ) noexcept;

    auto build_binary_representation(
        ice::Allocator& alloc,
        ice::Span<ice::ui::RawElement> raw_elements
    ) noexcept -> ice::Memory
    {
        ice::u32 const element_count = ice::size(raw_elements);

        ice::u8 type_info_counts[256]{ };
        ice::usize additional_data_size = 0;
        for (RawElement const& element : raw_elements)
        {
            type_info_counts[static_cast<ice::u32>(element.type)] += 1;
            if (element.type == ElementType::Button)
            {
                additional_data_size += reinterpret_cast<RawButtonInfo const*>(element.type_data)->text.size();
            }
        }

        auto const type_count = [&type_info_counts](ElementType type) noexcept -> ice::u32
        {
            return type_info_counts[static_cast<ice::u32>(type)];
        };

        ice::usize byte_size = sizeof(ice::ui::UIData);
        byte_size += element_count * sizeof(ice::ui::ElementInfo);
        byte_size += element_count * sizeof(ice::ui::Size);
        byte_size += element_count * sizeof(ice::ui::Position);
        byte_size += element_count * sizeof(ice::ui::RectOffset) * 2;
        byte_size += type_count(ElementType::Button) * sizeof(ice::ui::ButtonInfo) + alignof(ice::ui::ButtonInfo);
        byte_size += additional_data_size;

        ice::Memory const result{
            .location = alloc.allocate((ice::u32) byte_size, 16),
            .size = (ice::u32)byte_size,
            .alignment = 16
        };
        void const* const data_end = ice::memory::ptr_add(result.location, result.size);

        static auto store_span_info = [base_ptr = result.location](auto& span_value) noexcept
        {
            void* span_address = std::addressof(span_value);
            ice::u32 const span_size = ice::size(span_value);
            ice::u32 const span_offset = ice::memory::ptr_distance(base_ptr, span_value.data());

            ice::u32* values = reinterpret_cast<ice::u32*>(span_address);
            values[0] = span_offset;
            values[1] = span_size;
        };

        {
            using namespace ice::ui;

            UIData* isui = reinterpret_cast<UIData*>(result.location);
            ElementInfo* elements = reinterpret_cast<ElementInfo*>(isui + 1);
            Size* sizes = reinterpret_cast<Size*>(elements + element_count);
            Position* positions = reinterpret_cast<Position*>(sizes + element_count);
            RectOffset* margins = reinterpret_cast<RectOffset*>(positions + element_count);
            RectOffset* paddings = reinterpret_cast<RectOffset*>(margins + element_count);

            ButtonInfo* button_info = reinterpret_cast<ButtonInfo*>(
                ice::memory::ptr_align_forward(
                    paddings + element_count,
                    alignof(ButtonInfo)
                )
            );

            void* additional_data = button_info + type_count(ElementType::Button);

            ice::usize additional_data_offset = 0;

            isui->elements = { elements, element_count };
            isui->sizes = { sizes, element_count };
            isui->positions = { positions, element_count };
            isui->margins = { margins, element_count };
            isui->paddings = { paddings, element_count };
            isui->data_buttons = { button_info, type_count(ElementType::Button) };
            isui->additional_data = additional_data;
            //isui->data_label = { };
            //isui->data_button = { };

            ice::u8 type_data_index[256]{ };

            ice::u16 idx = 0;
            for (RawElement const& element : raw_elements)
            {
                u8& data_idx = type_data_index[static_cast<ice::u32>(element.type)];

                elements[idx].parent = element.parent;
                elements[idx].size_i = idx | static_cast<ice::u16>((static_cast<ice::u32>(element.size_flags) >> 0) << 12);
                elements[idx].pos_i = idx | static_cast<ice::u16>((static_cast<ice::u32>(element.position_flags) >> 4) << 12);
                elements[idx].mar_i = idx | static_cast<ice::u16>((static_cast<ice::u32>(element.margin_flags) >> 8) << 12);
                elements[idx].pad_i = idx | static_cast<ice::u16>((static_cast<ice::u32>(element.padding_flags) >> 8) << 12);
                elements[idx].type = static_cast<ice::ui::ElementType>(element.type);
                elements[idx].type_data_i = data_idx;

                sizes[idx] = element.size;
                positions[idx] = element.position;
                margins[idx] = element.margin;
                paddings[idx] = element.padding;

                if (element.type == ElementType::Button)
                {
                    RawButtonInfo const* const raw_button_info = reinterpret_cast<RawButtonInfo const*>(element.type_data);
                    ice::memcpy(additional_data, raw_button_info->text.data(), raw_button_info->text.size());

                    button_info[data_idx].text_offset = additional_data_offset;
                    button_info[data_idx].text_size = raw_button_info->text.size();

                    additional_data = ice::memory::ptr_add(additional_data, raw_button_info->text.size());
                    additional_data_offset += raw_button_info->text.size();
                }

                data_idx += 1;
                idx += 1;
            }

            ICE_ASSERT(
                additional_data <= data_end,
                "Moved past the allocated data buffer!"
            );

            store_span_info(isui->elements);
            store_span_info(isui->sizes);
            store_span_info(isui->positions);
            store_span_info(isui->margins);
            store_span_info(isui->paddings);
            store_span_info(isui->data_buttons);

            isui->additional_data = reinterpret_cast<void*>(
                static_cast<ice::uptr>(ice::memory::ptr_distance(isui, isui->additional_data))
            );
        }

        return result;
    }

    bool bake_isui_asset(
        void*,
        ice::Allocator& alloc,
        ice::ResourceTracker const&,
        ice::Resource_v2 const& resource,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept
    {
        // RapidXML requires writable data for in-situ parsing.
        void* data_copy = alloc.allocate(data.size, data.alignment);
        ice::memcpy(data_copy, data.location, data.size + 1);

        *reinterpret_cast<char*>(ice::memory::ptr_add(data_copy, data.size)) = '\0';

        {
            ice::pod::Array<ice::ui::RawElement> elements{ alloc };
            ice::pod::array::reserve(elements, 50);

            rapidxml_ns::xml_document<char>* doc = alloc.make<rapidxml_ns::xml_document<char>>();
            doc->parse<rapidxml_ns::parse_default>(reinterpret_cast<char*>(data_copy));

            compile_ui(alloc, *doc, elements);

            for (ice::ui::RawElement const& element : elements)
            {
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "type: {}", (int)element.type);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [w: {}, h: {}] size", (int)element.size.width, (int)element.size.height);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [x: {}, y: {}] position", (int)element.position.x, (int)element.position.y);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [l: {}, t: {}, r: {}, b: {}] margin", (int)element.margin.left, (int)element.margin.top, (int)element.margin.right, (int)element.margin.bottom);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [l: {}, t: {}, r: {}, b: {}] padding", (int)element.padding.left, (int)element.padding.top, (int)element.padding.right, (int)element.padding.bottom);
            }

            out_memory = build_binary_representation(alloc, elements);

            for (ice::ui::RawElement const& element : elements)
            {
                alloc.deallocate(element.type_data);
            }

            alloc.destroy(doc);
        }

        alloc.deallocate(data_copy);
        return true;
    }

    bool load_isui_asset(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& metadata,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept
    {
        ice::ui::UIData const* uidata = reinterpret_cast<ice::ui::UIData const*>(data.location);
        ice::ui::UIData* result_data = alloc.make<ice::ui::UIData>(*uidata);

        static auto restore_span_value = [base_ptr = data.location](auto& span_value) noexcept
        {
            void const* span_address = std::addressof(span_value);
            ice::u32 const* values = reinterpret_cast<ice::u32 const*>(span_address);

            ice::u32 const span_offset = values[0];
            ice::u32 const span_size = values[1];

            void const* span_data = ice::memory::ptr_add(base_ptr, span_offset);

            span_value = {
                reinterpret_cast<typename ice::clear_type_t<decltype(span_value)>::value_type const*>(span_data),
                span_size
            };
        };

        restore_span_value(result_data->elements);
        restore_span_value(result_data->sizes);
        restore_span_value(result_data->positions);
        restore_span_value(result_data->margins);
        restore_span_value(result_data->paddings);
        restore_span_value(result_data->data_buttons);
        result_data->additional_data = ice::memory::ptr_add(uidata, reinterpret_cast<ice::uptr>(uidata->additional_data));

        out_memory.location = result_data;
        out_memory.size = sizeof(ice::ui::UIData);
        out_memory.alignment = alignof(ice::ui::UIData);

        return true;
    }

    void register_ui_asset(ice::AssetTypeArchive& type_archive) noexcept
    {
        static ice::Utf8String asset_extensions[]{
            u8".isui"
        };

        static ice::AssetTypeDefinition asset_definition
        {
            .resource_extensions = asset_extensions,
            .fn_asset_oven = bake_isui_asset,
            .fn_asset_loader = load_isui_asset
        };

        type_archive.register_type(ice::ui::AssetType_UIPage, asset_definition);
    }

    void parse_element_size(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Size& size
    ) noexcept
    {
        using ice::ui::ElementFlags;

        auto const* const separator = std::find_if(it, end, [](char c) noexcept { return c == ','; });
        if (separator)
        {
            [[maybe_unused]]
            auto const res_w = std::from_chars(it, separator, size.width);

            [[maybe_unused]]
            auto const res_h = std::from_chars(separator + 1, end, size.height);

            //if (res_w; res_w.ec != std::errc{})
            //{
            //    if (strncmp(res_w.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Size_AutoWidth;
            //    }
            //    else if (strncmp(res_w.ptr, "*", 1) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Size_StretchWidth;
            //    }
            //}
            //if (res_h; res_h.ec != std::errc{})
            //{
            //    if (strncmp(res_h.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Size_AutoHeight;
            //    }
            //    else if (strncmp(res_h.ptr, "*", 1) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Size_StretchHeight;
            //    }
            //}
        }
    }

    void parse_element_pos(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Position& pos
    ) noexcept
    {
        auto const* const separator = std::find_if(it, end, [](char c) noexcept { return c == ','; });

        if (separator)
        {
            std::from_chars(it, separator, pos.x);
            std::from_chars(separator + 1, end, pos.y);
        }
    }

    void parse_element_offset(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::RectOffset& offset
    ) noexcept
    {
        using ice::ui::ElementFlags;

        char const* const sep1 = std::find_if(it, end, [](char c) noexcept { return c == ','; });
        char const* const sep2 = std::find_if(sep1 + 1, end, [](char c) noexcept { return c == ','; });
        char const* const sep3 = std::find_if(sep2 + 1, end, [](char c) noexcept { return c == ','; });

        if (sep1 && sep2 && sep3)
        {
            auto const res_l = std::from_chars(it, sep1, offset.left);
            auto const res_t = std::from_chars(sep1 + 1, sep2, offset.top);
            auto const res_r = std::from_chars(sep2 + 1, sep3, offset.right);
            auto const res_b = std::from_chars(sep3 + 1, end, offset.bottom);

            //if (res_l; res_l.ec != std::errc{})
            //{
            //    if (strncmp(res_l.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoLeft;
            //    }
            //}
            //if (res_t; res_t.ec != std::errc{})
            //{
            //    if (strncmp(res_t.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoTop;
            //    }
            //}
            //if (res_r; res_r.ec != std::errc{})
            //{
            //    if (strncmp(res_r.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoRight;
            //    }
            //}
            //if (res_b; res_b.ec != std::errc{})
            //{
            //    if (strncmp(res_b.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoBottom;
            //    }
            //}
        }
    }

    void compile_element_type(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* xml_element,
        ice::ui::RawElement& info
    ) noexcept
    {
        info.type = ElementType::Page;
        info.type_data = nullptr;

        if (strcmp(xml_element->local_name(), "button") == 0)
        {
            ice::ui::RawButtonInfo* button_info = reinterpret_cast<ice::ui::RawButtonInfo*>(alloc.allocate(sizeof(RawButtonInfo)));

            //rapidxml_ns::xml_node<char> const* xml_entity = xml_element->first_node_ns(
            //    Constant_ISUINamespaceIceShard.data(),
            //    Constant_ISUINamespaceIceShard.size(),
            //    "entity",
            //    6
            //);
            //if (xml_entity != nullptr)
            //{
            //    if (auto const* entity_name = xml_entity->first_attribute("name"); entity_name != nullptr)
            //    {
            //        button_info->entity_name = std::u8string_view{
            //            reinterpret_cast<ice::c8utf const*>(entity_name->value()),
            //            entity_name->value_size()
            //        };
            //    }
            //}

            if (auto const* attr = xml_element->first_attribute("text"))
            {
                button_info->text = std::u8string_view{ reinterpret_cast<ice::c8utf const*>(attr->value()), attr->value_size() };
            }
            else if (auto const* xml_node = xml_element->first_node("text"))
            {
                button_info->text = std::u8string_view{ reinterpret_cast<ice::c8utf const*>(xml_node->value()), xml_node->value_size() };
            }

            info.type_data = button_info;
            info.type = ice::ui::ElementType::Button;
        }
        //else if (strcmp(xml_element->local_name(), "label") == 0)
        //{
        //    ice::ui::LabelData* data = new (malloc(sizeof(ice::ui::LabelData))) ice::ui::LabelData{ };

        //    if (auto const* attr = xml_element->first_attribute("text"))
        //    {
        //        data->text = std::u8string_view{ reinterpret_cast<ice::utf8 const*>(attr->value()), attr->value_size() };
        //    }
        //    else if (auto const* xml_node = xml_element->first_node("text"))
        //    {
        //        data->text = std::u8string_view{ reinterpret_cast<ice::utf8 const*>(xml_node->value()), xml_node->value_size() };
        //    }

        //    info.type_data = data;
        //    info.type = 1;
        //}
    }

    void compile_element_attribs(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* element,
        ice::ui::RawElement& info
    ) noexcept
    {
        using ice::ui::ElementFlags;

        if (auto const* attr_size = element->first_attribute("size"); attr_size != nullptr)
        {
            parse_element_size(
                attr_size->value(),
                attr_size->value() + attr_size->value_size(),
                info.size_flags,
                info.size
            );
        }
        //else
        //{
        //    info.size_flags = ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight;
        //}

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

        compile_element_type(alloc, element, info);
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
            ice::ui::RawElement{ .parent = parent_idx }
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

    void compile_page(
        ice::Allocator& alloc,
        rapidxml_ns::xml_node<char> const* page,
        ice::pod::Array<RawElement>& elements
    ) noexcept
    {
        // TODO: Check!
        //ICE_ASSERT(
        //    ice::pod::array::size(elements) < ice::u32{ 1 << 12 },
        //    "Page has more element's than allowed!"
        //);

        compile_element(alloc, page, 0, elements);
    }

    void compile_ui(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::ui::RawElement>& raw_elements
    ) noexcept
    {
        rapidxml_ns::xml_node<char> const* root = doc.first_node("isui");
        if (root == nullptr)
        {
            return;
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

} // namespace ice::ui
