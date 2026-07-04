/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_context.hxx>
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/array.hxx>
#include <ice/engine_frame.hxx>
#include <ice/colors.hxx>

#include "widgets/imgui_devui_manager.hxx"
#include "widgets/imgui_logger.hxx"

namespace ice::devui
{

    struct ImGuiStats;

    class ImGui_ColorPicker_OkLCH : public ice::DevUIWidget
    {
    public:
        ImGui_ColorPicker_OkLCH() noexcept;

        void build_content() noexcept override;
        void update_state(ice::DevUIWidgetState& state) noexcept override;

    protected:
        ice::Color color = ice::color::Red;
        ice::Color newcolor = ice::color::Red;
    };

    class ImGuiWidgetFrame final : public ice::DevUIFrame
    {
    public:
        void mainmenu(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept override;
        bool begin(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept override;
        void end() noexcept override;
    };

    class ImGuiSystem final : public ice::DevUIContext
    {
    public:
        ImGuiSystem(ice::Allocator& alloc) noexcept;
        ~ImGuiSystem() noexcept override;

        void setup_mainmenu(ice::Span<ice::I18NReference> categories) noexcept;

        void register_widget(
            ice::DevUIWidget* widget,
            ice::DevUIWidget* owning_widget
        ) noexcept;
        void unregister_widget(ice::DevUIWidget* widget) noexcept;

        void update_widgets() noexcept override;

        auto allocator() noexcept -> ice::Allocator& { return _allocator.backing_allocator(); }

        auto logger() noexcept -> ice::devui::ImGuiLogger& { return _widget_logger; }

        void devui_draw(ice::devui::ImGuiStats const& stats) noexcept;

    private:
        ice::ProxyAllocator _allocator;
        ice::Array<ice::UniquePtr<ice::DevUIWidget>> _builtin_widgets;
        ice::Array<ice::I18NString> _menu_categories;

        ice::devui::ImGuiDevUIManager _widget_manager;
        ice::devui::ImGuiWidgetFrame _widget_frame;
        ice::devui::ImGuiLogger _widget_logger;
        ice::devui::ImGui_ColorPicker_OkLCH _widget_colorpicker;
        //ice::devui::ImGuiStylePalette _widget_style;
    };

} // namespace ice::devui
