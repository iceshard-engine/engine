#pragma once
#include <ice/devui/devui_system.hxx>

#include "imgui_trait.hxx"
#include "widgets/widget_imgui_allocator_tree.hxx"

namespace ice::devui
{

    class ImGuiSystem : public ice::devui::DevUISystem
    {
    public:
        ImGuiSystem(ice::Allocator& alloc) noexcept;
        ~ImGuiSystem() noexcept override;

        auto world_trait() noexcept -> ice::devui::DevUITrait* override;

        void register_widget(ice::devui::DevUIWidget* widget) noexcept override;
        void unregister_widget(ice::devui::DevUIWidget* widget) noexcept override;

        void set_alloc_tree_widget(ice::devui::ImGui_AllocatorTreeWidget* widget) noexcept;

        void internal_set_key(ice::devui::DevUIExecutionKey new_execution_key) noexcept override;
        void internal_build_widgets(
            ice::EngineFrame& frame,
            ice::devui::DevUIExecutionKey execution_key
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::devui::DevUIExecutionKey _execution_key;
        ice::devui::ImGuiTrait _render_trait;
        ice::devui::ImGui_AllocatorTreeWidget* _widget_alloc_tree;

        ice::pod::Array<ice::devui::DevUIWidget*> _widgets;
    };

} // namespace ice::devui
