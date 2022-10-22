/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui/devui_system.hxx>

#include "imgui_trait.hxx"
#include "widgets/widget_imgui_allocator_tree.hxx"

namespace ice::devui
{

    class ImGuiSystem final : public ice::devui::DevUISystem
    {
    public:
        ImGuiSystem(ice::Allocator& alloc) noexcept;
        ~ImGuiSystem() noexcept override;

        void register_trait(ice::WorldTraitArchive& archive) noexcept override;
        void set_trait(ice::devui::ImGuiTrait* trait) noexcept;

        void register_widget(ice::devui::DevUIWidget* widget) noexcept override;
        void unregister_widget(ice::devui::DevUIWidget* widget) noexcept override;

        void set_alloc_tree_widget(ice::devui::ImGui_AllocatorTreeWidget* widget) noexcept;

        void internal_set_key(ice::devui::DevUIExecutionKey new_execution_key) noexcept override;
        void internal_build_widgets(
            ice::EngineFrame& frame,
            ice::devui::DevUIExecutionKey execution_key
        ) noexcept override;

        auto allocator() noexcept -> ice::Allocator& { return _allocator; }

    private:
        ice::Allocator& _allocator;
        ice::devui::DevUIExecutionKey _execution_key;
        ice::devui::ImGuiTrait* _render_trait;
        ice::devui::ImGui_AllocatorTreeWidget* _widget_alloc_tree;

        ice::Array<ice::devui::DevUIWidget*> _widgets;
        ice::Array<ice::devui::DevUIWidget*> _inactive_widgets;
    };

} // namespace ice::devui
