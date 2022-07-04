#include <ice/ui_asset.hxx>
#include <ice/ui_data.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>

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

    auto build_binary_representation(
        ice::Allocator& alloc,
        ice::Span<ice::RawElement> raw_elements,
        ice::Span<ice::RawResource> raw_resources,
        ice::Span<ice::RawShard> raw_shards
    ) noexcept -> ice::Memory
    {
        ice::u32 const element_count = ice::size(raw_elements);

        ice::u32 count_fonts = 0;
        ice::u32 count_actions = 0;
        ice::u32 const count_resources = ice::size(raw_resources);
        ice::u32 const count_shards = ice::size(raw_shards);

        ice::u8 type_info_counts[256]{ };
        ice::usize additional_data_size = 0;
        for (RawElement const& element : raw_elements)
        {
            type_info_counts[static_cast<ice::u32>(element.type)] += 1;
            if (element.type == ElementType::Button)
            {
                RawButtonInfo const* button_data = reinterpret_cast<RawButtonInfo const*>(element.type_data);
                additional_data_size += button_data->text.size();

                if (button_data->action_on_click.action_type != ice::ui::ActionType::None)
                {
                    count_actions += 1;
                }
                if (button_data->action_text.action_type != ice::ui::ActionType::None)
                {
                    count_actions += 1;
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

        auto const type_count = [&type_info_counts](ElementType type) noexcept -> ice::u32
        {
            return type_info_counts[static_cast<ice::u32>(type)];
        };

        ice::usize byte_size = sizeof(ice::ui::UIData);
        byte_size += element_count * sizeof(ice::ui::ElementInfo);
        byte_size += element_count * sizeof(ice::ui::Size);
        byte_size += element_count * sizeof(ice::ui::Position);
        byte_size += element_count * sizeof(ice::ui::RectOffset) * 2;
        byte_size += count_fonts * sizeof(ice::ui::FontInfo);
        byte_size += count_resources * sizeof(ice::ui::ResourceInfo);
        byte_size += count_shards * sizeof(ice::ui::ShardInfo); // + alignof(ice::ui::ShardInfo);
        byte_size += count_actions * sizeof(ice::ui::Action);
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
            FontInfo* fonts = reinterpret_cast<FontInfo*>(paddings + element_count);
            ShardInfo* shards = reinterpret_cast<ShardInfo*>(fonts + count_fonts);
            Action* actions = reinterpret_cast<Action*>(shards + count_shards);
            ResourceInfo* resources = reinterpret_cast<ResourceInfo*>(actions + count_actions);

            ButtonInfo* button_info = reinterpret_cast<ButtonInfo*>(
                ice::memory::ptr_align_forward(
                    resources + count_resources,
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
            isui->fonts = { fonts, count_fonts };
            isui->ui_shards = { shards, count_shards };
            isui->ui_actions = { actions, count_actions };
            isui->ui_resources = { resources, count_resources };
            isui->additional_data = additional_data;

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

                if (element.type == ElementType::Button)
                {
                    RawButtonInfo const* const raw_button_info = reinterpret_cast<RawButtonInfo const*>(element.type_data);
                    ice::memcpy(additional_data, raw_button_info->text.data(), raw_button_info->text.size());

                    button_info[data_idx].font_i = find_font_idx(raw_button_info->font.data_source);
                    button_info[data_idx].text_offset = additional_data_offset;
                    button_info[data_idx].text_size = raw_button_info->text.size();
                    button_info[data_idx].action_on_click_i = ~ice::u16{};
                    button_info[data_idx].action_text_i = ~ice::u16{};

                    additional_data = ice::memory::ptr_add(additional_data, raw_button_info->text.size());
                    additional_data_offset += raw_button_info->text.size();

                    if (raw_button_info->action_on_click.action_type != ActionType::None)
                    {
                        button_info[data_idx].action_on_click_i = idx_action;

                        ice::ui::Action& action = actions[idx_action];
                        action.type = raw_button_info->action_on_click.action_type;
                        action.type_i = 0;
                        action.type_data = raw_button_info->action_on_click.data.data_type;
                        action.type_data_i = 0;

                        if (action.type == ActionType::Shard)
                        {
                            for (ice::u16 idx_shard = 0; idx_shard < count_shards; ++idx_shard)
                            {
                                if (raw_shards[idx_shard].ui_name == raw_button_info->action_on_click.action_value)
                                {
                                    action.type_i = idx_shard;
                                    break;
                                }
                            }

                            action.type_data = DataSource::ValueProperty;

                            if (raw_button_info->action_on_click.data.data_source == u8"entity")
                            {
                                action.type_data_i = static_cast<ice::u16>(Property::Entity);
                            }
                        }

                        idx_action += 1;
                    }

                    if (raw_button_info->action_text.action_type != ActionType::None)
                    {
                        button_info[data_idx].action_text_i = idx_action;

                        ice::ui::Action& action = actions[idx_action];
                        action.type = raw_button_info->action_text.action_type;
                        action.type_i = 0;
                        action.type_data = raw_button_info->action_text.data.data_type;
                        action.type_data_i = 0;

                        if (action.type == ActionType::Shard)
                        {
                            action.type_i = find_shard_idx(raw_button_info->action_text.action_value);
                            action.type_data_i = find_resource_idx(raw_button_info->action_text.data.data_source);
                        }
                        else if (action.type == ActionType::Data)
                        {
                            action.type_data_i = find_resource_idx(raw_button_info->action_text.action_value);
                        }

                        idx_action += 1;
                    }
                }

                data_idx += 1;
                idx += 1;
            }

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
            store_span_info(isui->data_buttons);
            store_span_info(isui->fonts);
            store_span_info(isui->ui_shards);
            store_span_info(isui->ui_actions);
            store_span_info(isui->ui_resources);

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

            compile_ui(alloc, *doc, elements, uires, uishards);

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
        ice::ui::UIData const* ui_data = reinterpret_cast<ice::ui::UIData const*>(data.location);

        ice::ui::UIData* const ui_result = alloc.make<ice::ui::UIData>();
        ice::memcpy(ui_result, ui_data, sizeof(ice::ui::UIData));

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
        restore_span_value(ui_result->data_buttons);
        ui_result->additional_data = ice::memory::ptr_add(ui_data, reinterpret_cast<ice::uptr>(ui_data->additional_data));

        out_memory.location = ui_result;
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

} // namespace ice::ui
