/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "ip_ui_oven.hxx"
#include <ice/math.hxx>
#include <ice/ui_style.hxx>

namespace ice
{

    struct RawElement
    {
        ice::u16 parent;
        ice::String style;
        ice::ui::Size size;
        ice::ui::Position position;
        ice::ui::RectOffset margin;
        ice::ui::RectOffset padding;

        ice::ui::ElementFlags flags;
        ice::ui::ElementType type;
        ice::Memory type_data;
    };

    struct RawStyle
    {
        ice::String name;
        ice::ui::ElementType target_element;
        ice::ui::ElementState target_element_states;

        ice::ui::StyleFlags flags;

        union StyleData
        {
            struct
            {
                ice::String name;
                ice::vec2f uvs[4];
            } texture;
            ice::ui::StyleColor color;
        };

        StyleData background;
    };

    struct RawResource
    {
        ice::String ui_name;
        ice::ui::ResourceType type;

        struct FontData
        {
            ice::String font_name;
            ice::u16 font_size;
            ice::u16 font_default : 1;
        };

        union
        {
            ice::u32 type_data;
            FontData font_data;
        };
    };

    struct RawShard
    {
        ice::String ui_name;
        ice::ShardID shard_name;
    };

} // namespace ice
