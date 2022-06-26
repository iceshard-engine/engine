#include <ice/ui_asset.hxx>
#include <ice/ui_data.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>

#include <ice/task_sync_wait.hxx>
#include <ice/asset_storage.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

#include <rapidxml_ns/rapidxml_ns.hpp>
#undef assert

namespace ice
{

    using ice::ui::ElementType;

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

    struct RawButtonInfo
    {
        ice::Utf8String text;
    };

    void compile_ui(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::pod::Array<ice::RawElement>& raw_elements
    ) noexcept;

    auto build_binary_representation(
        ice::Allocator& alloc,
        ice::Span<ice::RawElement> raw_elements
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
            .location = alloc.allocate((ice::u32)byte_size, 16),
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

    auto bake_isui_asset(
        void*,
        ice::Allocator& alloc,
        ice::ResourceTracker const&,
        ice::Resource_v2 const& resource,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept -> ice::Task<bool>
    {
        // RapidXML requires writable data for in-situ parsing.
        void* data_copy = alloc.allocate(data.size, data.alignment);
        ice::memcpy(data_copy, data.location, data.size + 1);

        *reinterpret_cast<char*>(ice::memory::ptr_add(data_copy, data.size)) = '\0';

        {
            ice::pod::Array<ice::RawElement> elements{ alloc };
            ice::pod::array::reserve(elements, 50);

            rapidxml_ns::xml_document<char>* doc = alloc.make<rapidxml_ns::xml_document<char>>();
            doc->parse<rapidxml_ns::parse_default>(reinterpret_cast<char*>(data_copy));

            compile_ui(alloc, *doc, elements);

            for (ice::RawElement const& element : elements)
            {
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "type: {}", (int)element.type);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [w: {}, h: {}] size", (int)element.size.width, (int)element.size.height);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [x: {}, y: {}] position", (int)element.position.x, (int)element.position.y);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [l: {}, t: {}, r: {}, b: {}] margin", (int)element.margin.left, (int)element.margin.top, (int)element.margin.right, (int)element.margin.bottom);
                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "- [l: {}, t: {}, r: {}, b: {}] padding", (int)element.padding.left, (int)element.padding.top, (int)element.padding.right, (int)element.padding.bottom);
            }

            out_memory = build_binary_representation(alloc, elements);

            for (ice::RawElement const& element : elements)
            {
                alloc.deallocate(element.type_data);
            }

            alloc.destroy(doc);
        }

        alloc.deallocate(data_copy);
        co_return true;
    }

    auto load_isui_asset(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage& storage,
        ice::Metadata const& metadata,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept -> ice::Task<bool>
    {
        ice::Asset default_font_asset = co_await storage.request(ice::AssetType_Font, u8"calibri", ice::AssetState::Loaded);
        if (ice::asset_check(default_font_asset, AssetState::Loaded) == false)
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "Couldn't load UI asset due to missing fonts!"
            );
            co_return false;
        }

        ice::Font const* font = reinterpret_cast<ice::Font const*>(default_font_asset.data.location);
        ice::ui::UIData const* ui_data = reinterpret_cast<ice::ui::UIData const*>(data.location);

        void* load_result_data = alloc.allocate(sizeof(ice::ui::UIData) + sizeof(ice::ui::UIFont) * 1 + alignof(ice::ui::UIFont));
        ice::ui::UIData* const ui_result = reinterpret_cast<ice::ui::UIData*>(load_result_data);
        ice::ui::UIFont* const ui_result_fonts = reinterpret_cast<ice::ui::UIFont*>(
            ice::memory::ptr_align_forward(ui_result + 1, alignof(ice::ui::UIFont))
        );

        ice::memcpy(ui_result, ui_data, sizeof(ice::ui::UIData));

        ui_result_fonts[0].font = font;
        ui_result_fonts[0].size = 16;

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

        ui_result->fonts = ice::Span<ice::ui::UIFont const>{ ui_result_fonts, 1 };
        restore_span_value(ui_result->elements);
        restore_span_value(ui_result->sizes);
        restore_span_value(ui_result->positions);
        restore_span_value(ui_result->margins);
        restore_span_value(ui_result->paddings);
        restore_span_value(ui_result->data_buttons);
        ui_result->additional_data = ice::memory::ptr_add(ui_data, reinterpret_cast<ice::uptr>(ui_data->additional_data));

        out_memory.location = load_result_data;
        out_memory.size = sizeof(ice::ui::UIData);
        out_memory.alignment = alignof(ice::ui::UIData);
        co_return true;
    }

    void asset_type_ui_definition(ice::AssetTypeArchive& type_archive) noexcept
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
            auto const res_w = std::from_chars(it, separator, size.width);
            auto const res_h = std::from_chars(separator + 1, end, size.height);

            if (res_w; res_w.ec != std::errc{})
            {
                if (strncmp(res_w.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoWidth;
                }
                else if (strncmp(res_w.ptr, "*", 1) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchWidth;
                }
            }
            if (res_h; res_h.ec != std::errc{})
            {
                if (strncmp(res_h.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoHeight;
                }
                else if (strncmp(res_h.ptr, "*", 1) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchHeight;
                }
            }
        }
    }

    void parse_element_pos(
        char const* it,
        char const* end,
        ice::ui::ElementFlags& out_flags,
        ice::ui::Position& pos
    ) noexcept
    {
        using ice::ui::ElementFlags;
        auto const* const separator = std::find_if(it, end, [](char c) noexcept { return c == ','; });

        if (separator)
        {
            auto const res_x = std::from_chars(it, separator, pos.x);
            auto const res_y = std::from_chars(separator + 1, end, pos.y);

            if (res_x; res_x.ec != std::errc{})
            {
                if (strncmp(res_x.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoX;
                }
            }
            else if (pos.x < 0)
            {
                out_flags = out_flags | ElementFlags::Position_AnchorRight;
                pos.x *= -1.f;
            }

            if (res_y; res_y.ec != std::errc{})
            {
                if (strncmp(res_y.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoY;
                }
            }
            else if (pos.y < 0)
            {
                out_flags = out_flags | ElementFlags::Position_AnchorBottom;
                pos.y *= -1.f;
            }
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
        ice::pod::Array<ice::RawElement>& raw_elements
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
