/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui/devui_system.hxx>
#include <ice/mem_allocator_proxy.hxx>

#include "imgui_trait.hxx"
#include "widgets/widget_imgui_allocator_tree.hxx"

namespace ice::devui
{

    class ImGuiSystem final : public ice::devui::DevUISystem
    {
    public:
        ImGuiSystem(ice::Allocator& alloc) noexcept;
        ~ImGuiSystem() noexcept override;

        void register_trait(ice::TraitArchive& archive) noexcept override;
        void set_trait(ice::devui::ImGuiTrait* trait) noexcept;

        void register_widget(ice::devui::DevUIWidget* widget) noexcept override;
        void unregister_widget(ice::devui::DevUIWidget* widget) noexcept override;

        void set_alloc_tree_widget(ice::devui::ImGui_AllocatorTreeWidget* widget) noexcept;

        void render_builtin_widgets(ice::EngineFrame& frame) noexcept override;

        auto allocator() noexcept -> ice::Allocator& { return _allocator; }

    private:
        ice::ProxyAllocator _allocator;
        ice::devui::ImGuiTrait* _render_trait;
        ice::devui::ImGui_AllocatorTreeWidget* _widget_alloc_tree;

        struct WidgetRuntimeInfo
        {
            ice::devui::DevUIWidget* widget;
            ice::devui::WidgetState* state;
        };

        ice::Array<WidgetRuntimeInfo> _widgets;
        ice::Array<ice::devui::DevUIWidget*> _inactive_widgets;
    };

} // namespace ice::devui
