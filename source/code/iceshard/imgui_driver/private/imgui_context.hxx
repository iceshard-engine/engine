#pragma once
#include <core/pointer.hxx>
#include <debugui/debugui.hxx>
#include <debugui/debugui_module.hxx>
#include <iceshard/renderer/render_module.hxx>

#include "imgui_inputs.hxx"
#include "imgui_renderer.hxx"

#include <imgui/imgui.h>

namespace iceshard::debug::imgui
{

    class DebugUIContext_ImGui : public DebugUIContext
    {
    public:
        DebugUIContext_ImGui(
            core::allocator& alloc,
            iceshard::Engine& engine
        ) noexcept;
        ~DebugUIContext_ImGui() noexcept;

        void register_ui(DebugUI* ui_object) noexcept override;

        void update(core::MessageBuffer const& messages) noexcept;

        void update(Frame&, Frame const&) noexcept override;

        void end_frame(Frame&, Frame const&) noexcept override;

        auto context_handle() const noexcept -> debugui_context_handle override
        {
            return _imgui_handle;
        }

        auto render_task_factory() noexcept -> RenderStageTaskFactory* override;

    private:
        core::allocator& _allocator;
        core::memory::unique_pointer<ImGuiInputs> _imgui_inputs;
        core::memory::unique_pointer<ImGuiRenderer> _imgui_renderer;

        core::pod::Array<DebugUI*> _debugui_objects;

        union
        {
            ImGuiContext* _imgui_context;
            debugui_context_handle _imgui_handle;
        };
        static_assert(sizeof(_imgui_context) == sizeof(_imgui_handle));
    };

} // namespace debugui::imgui
