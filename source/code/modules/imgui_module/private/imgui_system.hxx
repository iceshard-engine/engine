/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_context.hxx>
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/array.hxx>

namespace ice::devui
{

    class ImGuiWidgetFrame final : public ice::DevUIFrame
    {
    public:
        void mainmenu(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept override;
        bool begin(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept override;
        void end() noexcept override;
    };

    class ImGuiSystem final : public ice::DevUIContext
    {
    public:
        ImGuiSystem(ice::Allocator& alloc) noexcept;
        ~ImGuiSystem() noexcept override;

        void register_widget(ice::DevUIWidget* widget) noexcept;
        void unregister_widget(ice::DevUIWidget* widget) noexcept;

        void update_widgets() noexcept override;

        auto allocator() noexcept -> ice::Allocator& { return _allocator.backing_allocator(); }

    private:
        ice::ProxyAllocator _allocator;
        ice::Array<ice::UniquePtr<ice::DevUIWidget>> _builtin_widgets;

        struct WidgetRuntimeInfo : ice::DevUIWidgetState
        {
            ice::DevUIWidget* widget;
        };

        ice::devui::ImGuiWidgetFrame _widget_frame;
        ice::Array<WidgetRuntimeInfo> _widgets;
    };

} // namespace ice::devui
