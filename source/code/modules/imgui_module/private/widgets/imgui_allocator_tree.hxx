/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>

namespace ice::devui
{

    class ImGui_AllocatorTreeWidget : public ice::DevUIWidget
    {
    public:
        ImGui_AllocatorTreeWidget(ice::AllocatorDebugInfo const& alloc) noexcept;
        ~ImGui_AllocatorTreeWidget() noexcept override = default;

        void build_widget(ice::DevUIFrame& frame, ice::DevUIWidgetState& state) noexcept override;
        void build_content() noexcept override;

    private:
        ice::AllocatorDebugInfo const& _root_tracked_allocator;

        char _filter[32]{};
        bool _expanded;
    };

    auto create_allocator_tree_widget(
        ice::Allocator& allocator
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

} // namespace ice::devui
