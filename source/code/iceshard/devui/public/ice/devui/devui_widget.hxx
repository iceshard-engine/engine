/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>

namespace ice::devui
{

    struct WidgetSettings
    {
        ice::String menu_text;
        ice::String menu_category = "Uncategorized";
    };

    struct WidgetState
    {
        bool is_visible;
    };

    class DevUIWidget
    {
    public:
        virtual ~DevUIWidget() noexcept = default;

        virtual auto settings() const noexcept -> ice::devui::WidgetSettings const& = 0;

        virtual void on_prepare(void* context, ice::devui::WidgetState&) noexcept { }

        virtual void on_draw() noexcept = 0;
    };

} // namespace ice::devui
