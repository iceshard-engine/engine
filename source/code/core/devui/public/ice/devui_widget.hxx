/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_types.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    struct DevUIWidgetInfo
    {
        ice::String category;
        ice::String name;
    };

    class IDevUIWidget
    {
    public:
        virtual ~IDevUIWidget() noexcept = default;

        virtual void build_content() noexcept = 0;
        virtual auto name() const noexcept -> ice::String = 0;
        virtual auto category() const noexcept -> ice::String = 0;
    };

    // TODO: Rename to Window
    class DevUIWidget : public IDevUIWidget
    {
    public:
        DevUIWidget(ice::DevUIWidgetInfo const& info) noexcept;
        virtual ~DevUIWidget() noexcept = default;

        virtual auto name() const noexcept -> ice::String override { return widget_info.name; }
        virtual auto category() const noexcept -> ice::String override { return widget_info.category; }

        virtual void update_state(ice::DevUIWidgetState& state) noexcept { }

        virtual void build_widget(ice::DevUIFrame& frame, ice::DevUIWidgetState& state) noexcept;

        virtual void build_menu() noexcept;

        virtual bool build_mainmenu(ice::DevUIWidgetState& state) noexcept;

        ice::DevUIWidgetInfo const widget_info;
    };

} // namespace ice
