/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ui_types.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    struct PageInfo
    {
        ice::Span<ice::ui::ElementInfo const> elements;
        ice::Span<ice::ui::Size const> sizes;
        ice::Span<ice::ui::Position const> positions;
        ice::Span<ice::ui::RectOffset const> margins;
        ice::Span<ice::ui::RectOffset const> paddings;

        ice::Span<ice::ui::FontInfo const> fonts;
        ice::Span<ice::ui::StyleInfo const> styles;
        ice::Span<ice::ui::ShardInfo const> ui_shards;
        ice::Span<ice::ui::ActionInfo const> ui_actions;
        ice::Span<ice::ui::ConstantInfo const> ui_constants;
        ice::Span<ice::ui::ResourceInfo const> ui_resources;

        ice::Span<ice::ui::Layout const> data_layouts;
        ice::Span<ice::ui::LabelInfo const> data_labels;
        ice::Span<ice::ui::ButtonInfo const> data_buttons;
        void const* additional_data;
    };

} // namespace ice::ui
