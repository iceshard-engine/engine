/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ui_types.hxx>
#include <ice/ui_data_ref.hxx>
#include <ice/string/string.hxx>
#include <ice/span.hxx>
#include <ice/font.hxx>

namespace ice::ui
{

    struct ButtonInfo
    {
        ice::ui::DataRef text;
        ice::ui::DataRef font;

        ice::u16 action_on_click_i;
    };

    auto button_get_font(
        ice::ui::PageInfo const& data,
        ice::ui::ButtonInfo const& button_info,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::Font const*;

} // namespace ice::ui
