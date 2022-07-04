#include <ice/ui_asset.hxx>
#include <ice/ui_page.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>
#include <ice/ui_label.hxx>
#include <ice/ui_resource.hxx>
#include <ice/ui_font.hxx>
#include <ice/ui_shard.hxx>
#include <ice/ui_action.hxx>

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

    struct UISizeInfo
    {
        ice::u32 const count_elements;

        ice::u32 const count_fonts;
        ice::u32 const count_shards;
        ice::u32 const count_actions;
        ice::u32 const count_constants;
        ice::u32 const count_resources;

        ice::u32 const count_labels;
        ice::u32 const count_buttons;

        ice::u32 const total_required_space;
    };


    auto gather_counts_and_sizes(
        ice::Span<ice::RawElement> raw_elements,
        ice::Span<ice::RawResource> raw_resources,
        ice::Span<ice::RawShard> raw_shards
    ) noexcept -> ice::UISizeInfo
    {
        ice::u32 const count_elements = ice::size(raw_elements);
        ice::u32 const count_shards = ice::size(raw_shards);
        ice::u32 const count_resources = ice::size(raw_resources);

        ice::u32 count_fonts = 0;
        ice::u32 count_actions = 0;
        ice::u32 count_constants = 0;

        ice::u8 type_info_counts[256]{ };

        ice::u32 additional_data_size = 0;
        for (RawElement const& element : raw_elements)
        {
            type_info_counts[static_cast<ice::u32>(element.type)] += 1;
            if (element.type == ElementType::Button)
            {
                RawButtonInfo const* button_data = reinterpret_cast<RawButtonInfo const*>(element.type_data);
                if (button_data->action_on_click.action_type != ice::ui::ActionType::None)
                {
                    count_actions += 1;
                }
                if (button_data->text.data_type == ice::ui::DataSource::ValueConstant)
                {
                    additional_data_size += button_data->text.data_source.size();
                    count_constants += 1;
                }
            }
            else if (element.type == ElementType::Label)
            {
                RawLabelInfo const* label_data = reinterpret_cast<RawLabelInfo const*>(element.type_data);
                if (label_data->text.data_type == ice::ui::DataSource::ValueConstant)
                {
                    additional_data_size += label_data->text.data_source.size();
                    count_constants += 1;
                }
            }
        }

        for (ice::RawResource const& resource : raw_resources)
        {
            if (resource.type == ice::ui::ResourceType::Font)
            {
                count_fonts += 1;
                additional_data_size += resource.font_data.font_name.size();
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
        ice::Utf8String name
    ) noexcept -> ice::u16
    {
        ice::u16 idx = 0;
        ice::u16 const count = ice::size(resources);
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
        ice::u32 idx;
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

            ice::u32 const text_size = ice::string::size(raw_data_ref.data_source);
            constants.data[constants.idx].offset = constants.data_storage_offset;
            constants.data[constants.idx].size = text_size;
            ice::memcpy(
                constants.data_storage,
                ice::string::data(raw_data_ref.data_source),
                text_size
            );

            constants.data_storage = ice::memory::ptr_add(constants.data_storage, text_size);
            constants.data_storage_offset += text_size;
        }
        else if (out_ref.source == DataSource::ValueResource)
        {
            out_ref.source_i = find_resource_idx(resources, raw_data_ref.data_source);
        }
    }

    auto build_binary_representation(
        ice::Allocator& alloc,
        ice::Span<ice::RawElement> raw_elements,
        ice::Span<ice::RawResource> raw_resources,
        ice::Span<ice::RawShard> raw_shards
    ) noexcept -> ice::Memory
    {
        ice::UISizeInfo const ui_sizes = ice::gather_counts_and_sizes(raw_elements, raw_resources, raw_shards);
        ice::Memory const result{
            .location = alloc.allocate(ui_sizes.total_required_space, 16),
            .size = ui_sizes.total_required_space,
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

            PageInfo* isui = reinterpret_cast<PageInfo*>(result.location);
            ElementInfo* elements = reinterpret_cast<ElementInfo*>(isui + 1);
            Size* sizes = reinterpret_cast<Size*>(elements + ui_sizes.count_elements);
            Position* positions = reinterpret_cast<Position*>(sizes + ui_sizes.count_elements);
            RectOffset* margins = reinterpret_cast<RectOffset*>(positions + ui_sizes.count_elements);
            RectOffset* paddings = reinterpret_cast<RectOffset*>(margins + ui_sizes.count_elements);
            FontInfo* fonts = reinterpret_cast<FontInfo*>(paddings + ui_sizes.count_elements);
            ShardInfo* shards = reinterpret_cast<ShardInfo*>(fonts + ui_sizes.count_fonts);
            ActionInfo* actions = reinterpret_cast<ActionInfo*>(shards + ui_sizes.count_shards);
            ConstantInfo* constants = reinterpret_cast<ConstantInfo*>(actions + ui_sizes.count_actions);
            ResourceInfo* resources = reinterpret_cast<ResourceInfo*>(constants + ui_sizes.count_constants);

            LabelInfo* label_info = reinterpret_cast<LabelInfo*>(
                ice::memory::ptr_align_forward(
                    resources + ui_sizes.count_resources,
                    alignof(LabelInfo)
                )
            );
            ButtonInfo* button_info = reinterpret_cast<ButtonInfo*>(
                ice::memory::ptr_align_forward(
                    label_info + ui_sizes.count_labels,
                    alignof(ButtonInfo)
                )
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
            isui->ui_shards = { shards, ui_sizes.count_shards };
            isui->ui_actions = { actions, ui_sizes.count_actions };
            isui->ui_constants = { constants, ui_sizes.count_constants };
            isui->ui_resources = { resources, ui_sizes.count_resources };
            isui->data_labels = { label_info, ui_sizes.count_labels };
            isui->data_buttons = { button_info, ui_sizes.count_buttons };
            isui->additional_data = constants_ref.data_storage;

            auto const find_font_idx = [&raw_resources](ice::Utf8String font_name) noexcept -> ice::u16
            {
                ice::u16 idx = 0;
                ice::u16 font_idx = 0;
                ice::u16 const count = ice::size(raw_resources);
                for (; idx < count; ++idx)
                {
                    if (raw_resources[idx].type == ResourceType::Font)
                    {
                        if (raw_resources[idx].ui_name == font_name)
                        {
                            break;
                        }
                        font_idx += 1;
                    }
                }
                return idx == count ? ice::u16{ 0xff'ff } : font_idx;
            };

            auto const find_shard_idx = [&raw_shards](ice::Utf8String resource_name) noexcept -> ice::u16
            {
                ice::u16 idx = 0;
                ice::u16 const count = ice::size(raw_shards);
                for (; idx < count; ++idx)
                {
                    if (raw_shards[idx].ui_name == resource_name)
                    {
                        break;
                    }
                }
                return idx == count ? ice::u16{ 0 } : idx;
            };

            auto const find_resource_idx = [&raw_resources](ice::Utf8String resource_name) noexcept -> ice::u16
            {
                ice::u16 idx = 0;
                ice::u16 const count = ice::size(raw_resources);
                for (; idx < count; ++idx)
                {
                    if (raw_resources[idx].ui_name == resource_name)
                    {
                        break;
                    }
                }
                return idx == count ? ice::u16{0xffff} : idx;
            };

            ice::u8 type_data_index[256]{ };

            ice::u16 idx = 0;
            ice::u16 idx_action = 0;
            for (RawElement const& element : raw_elements)
            {
                u8& data_idx = type_data_index[static_cast<ice::u32>(element.type)];

                elements[idx].parent = element.parent;
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

                if (element.type == ElementType::Label)
                {
                    RawLabelInfo const* const raw_label_info = reinterpret_cast<RawLabelInfo const*>(element.type_data);

                    // Fonts can only be stored in resources.
                    label_info[data_idx].font.source = DataSource::ValueResource;
                    label_info[data_idx].font.source_i = find_font_idx(raw_label_info->font.data_source);

                    // Text info
                    ice::ui::DataRef& text_data = label_info[data_idx].text;
                    store_data_reference(raw_label_info->text, raw_resources, constants_ref, text_data);
                }
                else if (element.type == ElementType::Button)
                {
                    RawButtonInfo const* const raw_button_info = reinterpret_cast<RawButtonInfo const*>(element.type_data);

                    // Reset all actions.
                    button_info[data_idx].action_on_click_i = ~ice::u16{};

                    // Fonts can only be stored in resources.
                    button_info[data_idx].font.source = DataSource::ValueResource;
                    button_info[data_idx].font.source_i = find_font_idx(raw_button_info->font.data_source);

                    // Text info
                    ice::ui::DataRef& text_data = button_info[data_idx].text;
                    store_data_reference(raw_button_info->text, raw_resources, constants_ref, text_data);

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

            ice::u32 idx_res = 0;
            ice::u32 idx_font = 0;
            for (ice::RawResource const& resource : raw_resources)
            {
                resources[idx_res].id = ice::detail::stringid_type_v2::stringid<true>(resource.ui_name);
                resources[idx_res].type = resource.type;
                resources[idx_res].type_data = resource.type_data;

                if (resource.type == ResourceType::Font)
                {
                    fonts[idx_font].resource_i = idx_res;
                    fonts[idx_font].font_size = resource.font_data.font_size;
                    fonts[idx_font].font_name_offset = additional_data_offset;
                    fonts[idx_font].font_name_size = ice::string::size(resource.font_data.font_name);

                    ice::memcpy(additional_data, ice::string::data(resource.font_data.font_name), fonts[idx_font].font_name_size);

                    additional_data = ice::memory::ptr_add(additional_data, fonts[idx_font].font_name_size);
                    additional_data_offset += fonts[idx_font].font_name_size;

                    idx_font += 1;
                }

                idx_res += 1;
            }

            ice::u32 idx_shard = 0;
            for (ice::RawShard const& shard : raw_shards)
            {
                shards[idx_shard].shardid = ice::shard_id(shard.shard_name, ice::detail::Constant_ShardPayloadID<int>);
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
            store_span_info(isui->ui_shards);
            store_span_info(isui->ui_actions);
            store_span_info(isui->ui_resources);
            store_span_info(isui->data_labels);
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
            ice::pod::Array<ice::RawShard> uishards{ alloc };
            ice::pod::array::reserve(uishards, 25);
            ice::pod::array::push_back(uishards, RawShard{ });

            ice::pod::Array<ice::RawResource> uires{ alloc };
            ice::pod::array::reserve(uires, 25);

            ice::pod::Array<ice::RawElement> elements{ alloc };
            ice::pod::array::reserve(elements, 50);

            rapidxml_ns::xml_document<char>* doc = alloc.make<rapidxml_ns::xml_document<char>>();
            doc->parse<rapidxml_ns::parse_default>(reinterpret_cast<char*>(data_copy));

            parse_ui_file(alloc, *doc, elements, uires, uishards);

            out_memory = build_binary_representation(alloc, elements, uires, uishards);

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
        ice::ui::PageInfo const* ui_data = reinterpret_cast<ice::ui::PageInfo const*>(data.location);

        ice::ui::PageInfo* const ui_result = alloc.make<ice::ui::PageInfo>();
        ice::memcpy(ui_result, ui_data, sizeof(ice::ui::PageInfo));

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

        restore_span_value(ui_result->elements);
        restore_span_value(ui_result->sizes);
        restore_span_value(ui_result->positions);
        restore_span_value(ui_result->margins);
        restore_span_value(ui_result->paddings);
        restore_span_value(ui_result->fonts);
        restore_span_value(ui_result->ui_resources);
        restore_span_value(ui_result->ui_shards);
        restore_span_value(ui_result->ui_actions);
        restore_span_value(ui_result->data_labels);
        restore_span_value(ui_result->data_buttons);
        ui_result->additional_data = ice::memory::ptr_add(ui_data, reinterpret_cast<ice::uptr>(ui_data->additional_data));

        out_memory.location = ui_result;
        out_memory.size = sizeof(ice::ui::PageInfo);
        out_memory.alignment = alignof(ice::ui::PageInfo);
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

} // namespace ice::ui
