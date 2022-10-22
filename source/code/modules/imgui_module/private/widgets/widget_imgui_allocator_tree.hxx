/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/devui/devui_widget.hxx>

namespace ice::devui
{

    class ImGui_AllocatorTreeWidget : public ice::devui::DevUIWidget
    {
    public:
        ImGui_AllocatorTreeWidget(ice::AllocatorDebugInfo const& alloc) noexcept;
        ~ImGui_AllocatorTreeWidget() noexcept override = default;

        void on_draw() noexcept override;

    private:
        ice::AllocatorDebugInfo const& _root_tracked_allocator;
        bool _open = true;
    };

} // namespace ice::devui
