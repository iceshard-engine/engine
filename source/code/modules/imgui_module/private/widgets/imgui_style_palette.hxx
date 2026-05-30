/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/devui_widget.hxx>
#include <ice/colors.hxx>

namespace ice::devui
{

    struct DevUIStatusColors
    {
        ice::Color important;
        ice::Color failure;
        ice::Color warning;
        ice::Color hint;
        ice::Color neutral;
        ice::Color verbose;
    };

    static constexpr ice::devui::DevUIStatusColors Constant_DefaultStatusColors{
        .neutral = ice::Color{ 0.9f, 0.016f, 140.0_deg, 1.0f }
    };

    struct DevUITheme
    {
        ice::Color primary;
        ice::Color secondary;
        ice::Color accent;

        ice::devui::DevUIStatusColors status;

        struct Calculated
        {
            // Text
            ImColor text;
            ImColor text_disabled;
            // Window/Popup
            ImColor bg_window;
            ImColor bg_window_inner;
            ImColor bg_title;
            ImColor bg_title_focus;
            ImColor bg_title_disabled;
            ImColor border;
            ImColor border_shadow;
            // Scrollbar
            ImColor scroll_bg;
            ImColor scroll_neutral;
            ImColor scroll_hover;
            ImColor scroll_focus;
            // Table Widget
            ImColor table_header_bg;
            ImColor table_row0_bg;
            ImColor table_row1_bg;
            ImColor table_border_outer;
            ImColor table_border_inner;
            // Widgets
            ImColor bg_widget;
            ImColor bg_widget_hover;
            ImColor bg_widget_focus;
            // Menu
            ImColor bg_menu;
        } resolved;

        DevUITheme(
            ice::Color primary,
            ice::Color secondary,
            ice::Color accent
        ) noexcept;
    };


    namespace styles
    {

        enum class Theme : ice::u32
        {
            Dark = 0,
            Light,
        };

        void apply_color_theme(Theme theme) noexcept;
        void pop_color_theme() noexcept;

        auto apply_stylesheet() noexcept -> ice::u32;
        void pop_stylesheet() noexcept;

    } // namespace styles


    class ImGuiStylePalette : public ice::DevUIWidget
    {
    public:
        ImGuiStylePalette(ice::Allocator& alloc) noexcept;
        ~ImGuiStylePalette() noexcept override = default;

        void build_content() noexcept override;
    };

} // namespace ice::devui::styles
