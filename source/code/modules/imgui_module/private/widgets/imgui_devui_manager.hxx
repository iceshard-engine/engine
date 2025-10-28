/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/container/array.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice::devui
{

    struct ImGuiDevUIWidget
    {
        ice::DevUIWidgetState state;
        ice::DevUIWidget* widget;
        ice::u32 owner_index;
    };

    class ImGuiDevUIManager final : public ice::DevUIWidget
    {
    public:
        ImGuiDevUIManager(ice::Allocator& alloc) noexcept;
        ~ImGuiDevUIManager() noexcept;

        void add_widget(
            ice::DevUIWidget* widget,
            ice::DevUIWidget* owning_widget = nullptr
        ) noexcept;

        void remove_widget(ice::DevUIWidget* widget) noexcept;
        auto widgets() noexcept -> ice::Span<ice::UniquePtr<ImGuiDevUIWidget> const> { return ice::array::slice(_widgets, 1); }

        void build_content() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::Array<ice::UniquePtr<ImGuiDevUIWidget>> _widgets;
    };

} // namespace ice::devui
