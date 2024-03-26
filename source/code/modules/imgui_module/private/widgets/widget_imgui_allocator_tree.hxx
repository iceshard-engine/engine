/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/devui/devui_widget.hxx>

namespace ice::devui
{

    class ImGui_AllocatorTreeWidget : public ice::devui::DevUIWidget
    {
    public:
        ImGui_AllocatorTreeWidget(ice::AllocatorDebugInfo const& alloc) noexcept;
        ~ImGui_AllocatorTreeWidget() noexcept override = default;

        auto settings() const noexcept -> ice::devui::WidgetSettings const& override;

        void on_prepare(void*, ice::devui::WidgetState& state) noexcept override;

        void on_draw() noexcept override;

    private:
        ice::AllocatorDebugInfo const& _root_tracked_allocator;
        ice::devui::WidgetState* _state;

        char _filter[32];
        bool _expanded;
    };

} // namespace ice::devui
