/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ui_asset.hxx>
#include <ice/ui_page.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>
#include <ice/ui_label.hxx>
#include <ice/ui_resource.hxx>
#include <ice/ui_font.hxx>
#include <ice/ui_shard.hxx>
#include <ice/ui_action.hxx>
#include <ice/ui_style.hxx>

#include <ice/shard.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/asset_storage.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

#include <rapidxml_ns/rapidxml_ns.hpp>
#undef assert

#include "ip_ui_oven_page.hxx"
#include "ip_ui_oven_elements.hxx"

namespace ice
{

    struct UIRawInfo
    {
        ice::Span<ice::RawElement> elements;
        ice::Span<ice::RawResource> resources;
        ice::Span<ice::RawShard> shards;
        ice::Span<ice::RawStyle> styles;
    };

    struct UISizeInfo
    {
        ice::u32 const count_elements;

        ice::u32 const count_fonts;
        ice::u32 const count_styles;
        ice::u32 const count_shards;
        ice::u32 const count_actions;
        ice::u32 const count_constants;
        ice::u32 const count_resources;

        ice::u32 const count_labels;
        ice::u32 const count_buttons;

        ice::u32 const total_required_space;
    };

    auto gather_counts_and_sizes(
        ice::UIRawInfo const& raw_info
    ) noexcept -> ice::UISizeInfo
    {
        ice::u32 const count_elements = ice::count(raw_info.elements);
        ice::u32 const count_shards = ice::count(raw_info.shards);
        ice::u32 const count_resources = ice::count(raw_info.resources);
        ice::u32 const count_styles = ice::count(raw_info.styles);

        ice::u32 count_fonts = 0;
        ice::u32 count_actions = 0;
        ice::u32 count_constants = 0;

        ice::u8 type_info_counts[256]{ };

        ice::u32 additional_data_size = 0;
        for (RawElement const& element : raw_info.elements)
        {
            type_info_counts[static_cast<ice::u32>(element.type)] += 1;
            if (element.type == ElementType::Button)
            {
                RawButtonInfo const* button_data = reinterpret_cast<RawButtonInfo const*>(element.type_data.location);
                if (button_data->action_on_click.action_type != ice::ui::ActionType::None)
                {
                    count_actions += 1;
                }
                if (button_data->text.data_type == ice::ui::DataSource::ValueConstant)
                {
                    count_constants += 1;
                    additional_data_size += ice::string::size(button_data->text.data_source);
                }
            }
            else if (element.type == ElementType::Label)
            {
                RawLabelInfo const* label_data = reinterpret_cast<RawLabelInfo const*>(element.type_data.location);
                if (label_data->text.data_type == ice::ui::DataSource::ValueConstant)
                {
                    count_constants += 1;
                    additional_data_size += ice::string::size(label_data->text.data_source);
                }
            }
        }

        for (ice::RawResource const& resource : raw_info.resources)
        {
            if (resource.type == ice::ui::ResourceType::Font)
            {
                count_fonts += 1;
                additional_data_size += ice::string::size(resource.font_data.font_name);
            }
        }

        for (ice::RawStyle const& style : raw_info.styles)
        {
            if (has_any(style.flags, ice::ui::StyleFlags::BackgroundColor))
            {
                count_constants += 1;
                additional_data_size += sizeof(ice::ui::StyleColor);
            }
        }

        auto const type_count = [&type_info_counts](ice::ui::ElementType type) noexcept -> ice::u32
        {
            return type_info_counts[static_cast<ice::u32>(type)];
        };

        auto const type_size = [&type_count]<typename T>(T*, ice::ui::ElementType type) noexcept -> ice::u32
        {
            return type_count(type) * sizeof(T) + alignof(T);
        };

        ice::u32 byte_size = sizeof(ice::ui::PageInfo);
        byte_size += count_elements * sizeof(ice::ui::ElementInfo);
        byte_size += count_elements * sizeof(ice::ui::Size);
        byte_size += count_elements * sizeof(ice::ui::Position);
        byte_size += count_elements * sizeof(ice::ui::RectOffset) * 2;
        byte_size += count_fonts * sizeof(ice::ui::FontInfo);
        byte_size += count_styles *sizeof(ice::ui::StyleInfo);
        byte_size += count_shards * sizeof(ice::ui::ShardInfo);
        byte_size += count_actions * sizeof(ice::ui::ActionInfo);
        byte_size += count_constants * sizeof(ice::ui::ConstantInfo);
        byte_size += count_resources * sizeof(ice::ui::ResourceInfo);
        byte_size += type_size((ice::ui::LabelInfo*)nullptr, ElementType::Label);
        byte_size += type_size((ice::ui::ButtonInfo*)nullptr, ElementType::Button);
        byte_size += additional_data_size;

        return UISizeInfo{
            .count_elements = count_elements,
            .count_fonts = count_fonts,
            .count_styles = count_styles,
            .count_shards = count_shards,
            .count_actions = count_actions,
            .count_constants = count_constants,
            .count_resources = count_resources,
            .count_labels = type_count(ElementType::Label),
            .count_buttons = type_count(ElementType::Button),
            .total_required_space = byte_size,
        };
    }

    auto find_resource_idx(
        ice::Span<ice::RawResource> resources,
        ice::String name
    ) noexcept -> ice::u16
    {
        ice::u16 idx = 0;
        ice::u16 const count = ice::u16(ice::count(resources));
        for (; idx < count; ++idx)
        {
            if (resources[idx].ui_name == name)
            {
                break;
            }
        }
        return idx == count ? ice::u16{ 0xffff } : idx;
    }

    struct ConstantData
    {
        ice::u16 idx;
        ice::ui::ConstantInfo* data;
        ice::u32 data_storage_offset;
        void* data_storage;
    };

    auto store_data_reference(
        ice::RawData const& raw_data_ref,
        ice::Span<ice::RawResource> resources,
        ice::ConstantData& constants,
        ice::ui::DataRef& out_ref
    ) noexcept
    {
        using ice::ui::DataSource;

        out_ref.source = raw_data_ref.data_type;
        if (out_ref.source == DataSource::ValueConstant)
        {
            out_ref.source_i = constants.idx;

            ice::ucount const text_size = ice::string::size(raw_data_ref.data_source);
            constants.data[constants.idx].offset = constants.data_storage_offset;
            constants.data[constants.idx].size = text_size;
            ice::memcpy(
                constants.data_storage,
                ice::string::begin(raw_data_ref.data_source),
                text_size
            );

            constants.data_storage = ice::ptr_add(constants.data_storage, { text_size });
            constants.data_storage_offset += text_size;
            constants.idx += 1;
        }
        else if (out_ref.source == DataSource::ValueResource)
        {
            out_ref.source_i = find_resource_idx(resources, raw_data_ref.data_source);
        }
    }

    auto build_binary_representation(
        ice::Allocator& alloc,
        ice::UIRawInfo const& raw_info
    ) noexcept -> ice::Memory
    {
        ice::UISizeInfo const ui_sizes = ice::gather_counts_and_sizes(raw_info);
        ice::Memory const result = alloc.allocate({ { ui_sizes.total_required_space }, ice::ualign::b_16 });

        void const* const data_end = ice::ptr_add(result.location, result.size);

        static auto store_span_info = [base_ptr = result.location](auto& span_value) noexcept
        {
            void* span_address = std::addressof(span_value);
            ice::u32 const span_size = ice::count(span_value);
            ice::u32 const span_offset = ice::u32(ice::ptr_distance(base_ptr, ice::span::data(span_value)).value);

            ice::u32* values = reinterpret_cast<ice::u32*>(span_address);
            values[0] = span_offset;
            values[1] = span_size;
        };

        {
            using namespace ice::ui;

            PageInfo* isui = reinterpret_cast<PageInfo*>(result.location);
            ElementInfo* elements = reinterpret_cast<ElementInfo*>(isui + 1);
            Size* sizes = reinterpret_cast<Size*>(elements + ui_sizes.count_elements);
            Position* positions = reinterpret_cast<Position*>(sizes + ui_sizes.count_elements);
            RectOffset* margins = reinterpret_cast<RectOffset*>(positions + ui_sizes.count_elements);
            RectOffset* paddings = reinterpret_cast<RectOffset*>(margins + ui_sizes.count_elements);
            FontInfo* fonts = reinterpret_cast<FontInfo*>(paddings + ui_sizes.count_elements);
            StyleInfo* styles = reinterpret_cast<StyleInfo*>(fonts + ui_sizes.count_fonts);
            ShardInfo* shards = reinterpret_cast<ShardInfo*>(styles + ui_sizes.count_styles);
            ActionInfo* actions = reinterpret_cast<ActionInfo*>(shards + ui_sizes.count_shards);
            ConstantInfo* constants = reinterpret_cast<ConstantInfo*>(actions + ui_sizes.count_actions);
            ResourceInfo* resources = reinterpret_cast<ResourceInfo*>(constants + ui_sizes.count_constants);

            LabelInfo* label_info = reinterpret_cast<LabelInfo*>(
                ice::align_to(resources + ui_sizes.count_resources, ice::align_of<LabelInfo>).value
            );
            ButtonInfo* button_info = reinterpret_cast<ButtonInfo*>(
                ice::align_to(label_info + ui_sizes.count_labels, ice::align_of<ButtonInfo>).value
            );

            ice::ConstantData constants_ref{
                .idx = 0,
                .data = constants,
                .data_storage_offset = 0,
                .data_storage = button_info + ui_sizes.count_buttons,
            };

            isui->elements = { elements, ui_sizes.count_elements };
            isui->sizes = { sizes, ui_sizes.count_elements };
            isui->positions = { positions, ui_sizes.count_elements };
            isui->margins = { margins, ui_sizes.count_elements };
            isui->paddings = { paddings, ui_sizes.count_elements };
            isui->fonts = { fonts, ui_sizes.count_fonts };
            isui->styles = { styles, ui_sizes.count_styles };
            isui->ui_shards = { shards, ui_sizes.count_shards };
            isui->ui_actions = { actions, ui_sizes.count_actions };
            isui->ui_constants = { constants, ui_sizes.count_constants };
            isui->ui_resources = { resources, ui_sizes.count_resources };
            isui->data_labels = { label_info, ui_sizes.count_labels };
            isui->data_buttons = { button_info, ui_sizes.count_buttons };
            isui->additional_data = constants_ref.data_storage;

            auto const find_font_idx = [&raw_info](ice::String font_name) noexcept -> ice::u16
            {
                ice::u16 font_idx = 0;
                ice::u32 idx = 0;
                ice::u32 const count = ice::count(raw_info.resources);
                for (; idx < count; ++idx)
                {
                    if (raw_info.resources[idx].type == ResourceType::Font)
                    {
                        if (raw_info.resources[idx].ui_name == font_name)
                        {
                            break;
                        }
                        font_idx += 1;
                    }
                }
                return idx == count ? ice::u16{ 0xff'ff } : font_idx;
            };

            auto const find_shard_idx = [&raw_info](ice::String resource_name) noexcept -> ice::u16
            {
                ice::u16 idx = 0;
                ice::u32 const count = ice::count(raw_info.shards);
                for (; idx < count; ++idx)
                {
                    if (raw_info.shards[idx].ui_name == resource_name)
                    {
                        break;
                    }
                }
                return idx == count ? ice::u16{ 0 } : idx;
            };

            ice::u8 type_data_index[256]{ };

            ice::u16 idx = 0;
            ice::u16 idx_action = 0;
            for (RawElement const& element : raw_info.elements)
            {
                u8& data_idx = type_data_index[static_cast<ice::u32>(element.type)];

                elements[idx].parent = element.parent;
                elements[idx].style_i = 0;
                elements[idx].size_i = idx;
                elements[idx].pos_i = idx;
                elements[idx].mar_i = idx;
                elements[idx].pad_i = idx;
                elements[idx].flags = element.flags;
                elements[idx].type = static_cast<ice::ui::ElementType>(element.type);
                elements[idx].type_data_i = data_idx;

                sizes[idx] = element.size;
                positions[idx] = element.position;
                margins[idx] = element.margin;
                paddings[idx] = element.padding;

                // Find the style attached
                {
                    ice::u16 idx_by_name = 1;
                    ice::u16 idx_by_target = 0;
                    for (; idx_by_name < ui_sizes.count_styles; ++idx_by_name)
                    {
                        if (raw_info.styles[idx_by_name].target_element_states != ice::ui::ElementState::Any)
                        {
                            continue;
                        }
                        if (raw_info.styles[idx_by_name].name == element.style)
                        {
                            break;
                        }
                        if (raw_info.styles[idx_by_name].target_element == element.type)
                        {
                            idx_by_target = idx_by_name;
                        }
                    }

                    if (idx_by_name == ui_sizes.count_styles)
                    {
                        idx_by_name = idx_by_target;
                    }
                    elements[idx].style_i = idx_by_name;
                }

                if (element.type == ElementType::Label)
                {
                    RawLabelInfo const* const raw_label_info = reinterpret_cast<RawLabelInfo const*>(element.type_data.location);

                    // Fonts can only be stored in resources.
                    label_info[data_idx].font.source = DataSource::ValueResource;
                    label_info[data_idx].font.source_i = find_font_idx(raw_label_info->font.data_source);

                    // Text info
                    ice::ui::DataRef& text_data = label_info[data_idx].text;
                    store_data_reference(raw_label_info->text, raw_info.resources, constants_ref, text_data);
                }
                else if (element.type == ElementType::Button)
                {
                    RawButtonInfo const* const raw_button_info = reinterpret_cast<RawButtonInfo const*>(element.type_data.location);

                    // Reset all actions.
                    button_info[data_idx].action_on_click_i = ice::u16_max;

                    // Fonts can only be stored in resources.
                    button_info[data_idx].font.source = DataSource::ValueResource;
                    button_info[data_idx].font.source_i = find_font_idx(raw_button_info->font.data_source);

                    // Text info
                    ice::ui::DataRef& text_data = button_info[data_idx].text;
                    store_data_reference(raw_button_info->text, raw_info.resources, constants_ref, text_data);

                    if (raw_button_info->action_on_click.action_type != ActionType::None)
                    {
                        button_info[data_idx].action_on_click_i = idx_action;

                        ice::ui::ActionInfo& action = actions[idx_action];
                        action.type = raw_button_info->action_on_click.action_type;
                        action.type_i = 0;

                        ICE_ASSERT(
                            raw_button_info->action_on_click.data.data_type == DataSource::None,
                            "Not implemented!"
                        );

                        action.data.source = DataSource::None;
                        action.data.source_i = 0;

                        if (action.type == ActionType::Shard)
                        {
                            action.type_i = find_shard_idx(raw_button_info->action_on_click.action_value);
                        }

                        idx_action += 1;
                    }
                }

                data_idx += 1;
                idx += 1;
            }

            // Update the pointers properly.
            ice::u32 additional_data_offset = constants_ref.data_storage_offset;
            void* additional_data = constants_ref.data_storage;

            ice::u16 idx_res = 0;
            ice::u32 idx_font = 0;
            for (ice::RawResource const& resource : raw_info.resources)
            {
                // TODO: Force it to <debug string id>?
                resources[idx_res].id = ice::stringid(std::string_view{ resource.ui_name._data, resource.ui_name._size });
                resources[idx_res].type = resource.type;
                resources[idx_res].type_data = resource.type_data;

                if (resource.type == ResourceType::Font)
                {
                    fonts[idx_font].resource_i = idx_res;
                    fonts[idx_font].font_size = resource.font_data.font_size;
                    fonts[idx_font].font_name_offset = additional_data_offset;
                    fonts[idx_font].font_name_size = ice::string::size(resource.font_data.font_name);

                    ice::memcpy(additional_data, ice::string::begin(resource.font_data.font_name), fonts[idx_font].font_name_size);

                    additional_data = ice::ptr_add(additional_data, { fonts[idx_font].font_name_size });
                    additional_data_offset += fonts[idx_font].font_name_size;

                    idx_font += 1;
                }

                idx_res += 1;
            }

            void* aligned_ptr = ice::align_to(additional_data, ice::ualign::b_4).value;
            additional_data_offset += ice::ucount(ice::ptr_distance(additional_data, aligned_ptr).value);
            additional_data = aligned_ptr;

            ice::u32 idx_style = 0;
            for (ice::RawStyle const& style : raw_info.styles)
            {
                styles[idx_style] = StyleInfo{ .target_state = ElementState::Any, .flags = style.flags };

                if (has_any(style.flags, StyleFlags::BackgroundColor))
                {
                    styles[idx_style].target_state = style.target_element_states;
                    styles[idx_style].data_bg.source = DataSource::ValueConstant;
                    styles[idx_style].data_bg.source_i = constants_ref.idx;

                    ice::ui::ConstantInfo& constant_ref = constants_ref.data[constants_ref.idx];
                    constant_ref.offset = additional_data_offset;
                    constant_ref.size = sizeof(ice::ui::StyleColor);

                    ice::memcpy(additional_data, ice::addressof(style.background.color), constant_ref.size);

                    additional_data = ice::ptr_add(additional_data, { constant_ref.size });
                    additional_data_offset += constant_ref.size;
                    constants_ref.idx += 1;
                }

                idx_style += 1;
            }

            ice::u32 idx_shard = 0;
            for (ice::RawShard const& shard : raw_info.shards)
            {
                shards[idx_shard].shardid = shard.shard_name;
                shards[idx_shard].shardid.payload = { };
                idx_shard += 1;
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
            store_span_info(isui->fonts);
            store_span_info(isui->styles);
            store_span_info(isui->ui_shards);
            store_span_info(isui->ui_actions);
            store_span_info(isui->ui_constants);
            store_span_info(isui->ui_resources);
            store_span_info(isui->data_labels);
            store_span_info(isui->data_buttons);

            isui->additional_data = std::bit_cast<void*>(ice::ptr_distance(isui, isui->additional_data));
        }

        return result;
    }

    auto bake_isui_asset(
        void*,
        ice::Allocator& alloc,
        ice::ResourceTracker const&,
        ice::LooseResource const& resource,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept -> ice::Task<bool>
    {
        // RapidXML requires writable data for in-situ parsing.
        ice::Memory data_copy = alloc.allocate({ data.size + 1_B, data.alignment });
        ice::memcpy(data_copy, { data.location, data.size + 1_B });

        *reinterpret_cast<char*>(ice::ptr_add(data_copy.location, data.size)) = '\0';

        {
            ice::Array<ice::RawShard> uishards{ alloc };
            ice::array::reserve(uishards, 25);
            ice::array::push_back(uishards, RawShard{ });

            ice::Array<ice::RawResource> uires{ alloc };
            ice::array::reserve(uires, 25);

            ice::Array<ice::RawStyle> styles{ alloc };
            ice::array::reserve(styles, 25);
            ice::array::push_back(styles, RawStyle{ .flags = ice::ui::StyleFlags::None });

            ice::Array<ice::RawElement> elements{ alloc };
            ice::array::reserve(elements, 50);

            rapidxml_ns::xml_document<char>* doc = alloc.create<rapidxml_ns::xml_document<char>>();
            doc->parse<rapidxml_ns::parse_default>(reinterpret_cast<char*>(data_copy.location));

            parse_ui_file(alloc, *doc, elements, uires, uishards, styles);

            ice::UIRawInfo const raw_info{ elements, uires, uishards, styles };

            out_memory = build_binary_representation(alloc, raw_info);

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
        ice::Asset default_font_asset = co_await storage.request(ice::AssetType_Font, "local/font/calibri", ice::AssetState::Loaded);
        if (ice::asset_check(default_font_asset, AssetState::Loaded) == false)
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "Couldn't load UI asset due to missing fonts!"
            );
            co_return false;
        }

        ice::ui::PageInfo const* ui_data = reinterpret_cast<ice::ui::PageInfo const*>(data.location);

        ice::ui::PageInfo* const ui_result = alloc.create<ice::ui::PageInfo>();
        ice::memcpy(ui_result, ui_data, sizeof(ice::ui::PageInfo));

        static auto restore_span_value = [base_ptr = data.location](auto& span_value) noexcept
        {
            void const* span_address = std::addressof(span_value);
            ice::u32 const* values = reinterpret_cast<ice::u32 const*>(span_address);

            ice::u32 const span_offset = values[0];
            ice::u32 const span_size = values[1];

            void const* span_data = ice::ptr_add(base_ptr, { span_offset });

            span_value = {
                reinterpret_cast<typename ice::clear_type_t<decltype(span_value)>::ValueType const*>(span_data),
                span_size
            };
        };

        restore_span_value(ui_result->elements);
        restore_span_value(ui_result->sizes);
        restore_span_value(ui_result->positions);
        restore_span_value(ui_result->margins);
        restore_span_value(ui_result->paddings);
        restore_span_value(ui_result->fonts);
        restore_span_value(ui_result->styles);
        restore_span_value(ui_result->ui_shards);
        restore_span_value(ui_result->ui_actions);
        restore_span_value(ui_result->ui_constants);
        restore_span_value(ui_result->ui_resources);
        restore_span_value(ui_result->data_labels);
        restore_span_value(ui_result->data_buttons);
        ui_result->additional_data = ice::ptr_add(ui_data, std::bit_cast<ice::usize>(ui_data->additional_data));

        out_memory.location = ui_result;
        out_memory.size = ice::size_of<ice::ui::PageInfo>;
        out_memory.alignment = ice::align_of<ice::ui::PageInfo>;
        co_return true;
    }

    void asset_type_ui_definition(ice::AssetTypeArchive& type_archive) noexcept
    {
        static ice::String asset_extensions[]{
            ".isui"
        };

        static ice::AssetTypeDefinition asset_definition
        {
            .resource_extensions = asset_extensions,
            .fn_asset_oven = bake_isui_asset,
            .fn_asset_loader = load_isui_asset
        };

        type_archive.register_type(ice::ui::AssetType_UIPage, asset_definition);
    }

} // namespace ice::ui
