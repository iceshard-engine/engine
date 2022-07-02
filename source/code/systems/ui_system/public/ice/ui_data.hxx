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
        Utf8String,
    };

    struct ResourceInfo
    {
        ice::ui::ResourceType type;
        ice::u32 type_data;
    };

    struct UIFont
    {
        ice::Font const* font;
        ice::u32 size;
        // bool is_bold;
        // bool is_italic;
    };

    struct UIData
    {
        ice::Span<ice::ui::UIFont const> fonts;
        ice::Span<ice::ui::ElementInfo const> elements;
        ice::Span<ice::ui::Size const> sizes;
        ice::Span<ice::ui::Position const> positions;
        ice::Span<ice::ui::RectOffset const> margins;
        ice::Span<ice::ui::RectOffset const> paddings;
        ice::Span<ice::ui::ButtonInfo const> data_buttons;

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
