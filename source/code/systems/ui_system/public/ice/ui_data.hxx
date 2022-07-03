#pragma once
#include <ice/ui_types.hxx>
#include <ice/ui_shard.hxx>
#include <ice/ui_action.hxx>
#include <ice/font.hxx>

namespace ice::ui
{

    enum class ResourceType : ice::u32
    {
        None,
        Font,
        Utf8String,
    };

    struct ResourceInfo
    {
        ice::detail::stringid_type_v2::StringID<true> id;
        ice::ui::ResourceType type;
        ice::u32 type_data;
    };

    struct FontInfo
    {
        ice::usize font_name_offset;
        ice::u32 font_name_size;
        ice::u16 font_size;

        ice::u16 resource_i;
    };

    struct UIData
    {
        ice::Span<ice::ui::ElementInfo const> elements;
        ice::Span<ice::ui::Size const> sizes;
        ice::Span<ice::ui::Position const> positions;
        ice::Span<ice::ui::RectOffset const> margins;
        ice::Span<ice::ui::RectOffset const> paddings;
        ice::Span<ice::ui::ButtonInfo const> data_buttons;

        ice::Span<ice::ui::FontInfo const> fonts;
        ice::Span<ice::ui::ShardInfo const> ui_shards;
        ice::Span<ice::ui::Action const> ui_actions;
        ice::Span<ice::ui::ResourceInfo const> ui_resources;
        void const* additional_data;
    };

    struct UIResourceData
    {
        ice::ui::ResourceInfo info;
        void* location;
    };

} // namespace ice::ui
