/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/container/array.hxx>

namespace ice::devui
{

    struct ImGuiDevUIWidget : ice::DevUIWidgetState
    {
        ice::DevUIWidget* widget;
    };

    class ImGuiDevUIManager final : public ice::DevUIWidget
    {
    public:
        ImGuiDevUIManager(ice::Allocator& alloc) noexcept;
        ~ImGuiDevUIManager() noexcept;

        void add_widget(ice::DevUIWidget* widget) noexcept;
        void remove_widget(ice::DevUIWidget* widget) noexcept;
        auto widgets() noexcept -> ice::Span<ImGuiDevUIWidget> { return _widgets; }

        void build_content() noexcept override;

    private:
        ice::Array<ImGuiDevUIWidget> _widgets;
    };

} // namespace ice::devui
