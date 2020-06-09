#pragma once
#include <core/pointer.hxx>
#include <iceshard/debug/debug_system.hxx>
#include <iceshard/debug/debug_window.hxx>
#include <iceshard/debug/debug_module.hxx>
#include <iceshard/renderer/render_module.hxx>

#include "imgui_inputs.hxx"
#include "imgui_renderer.hxx"

#include <imgui/imgui.h>

namespace iceshard::debug::imgui
{

    class ImGuiDebugSystem : public ComponentSystem
    {
    public:
        ImGuiDebugSystem(core::allocator& alloc, iceshard::Engine& engine) noexcept;
        ~ImGuiDebugSystem() noexcept override;

        void update(Frame&, Frame const&) noexcept override;

        void end_frame(Frame&, Frame const&) noexcept override;

        auto render_task_factory() noexcept -> RenderStageTaskFactory* override;

        void add_window(
            core::stringid_arg_type name,
            DebugWindow* window
        ) noexcept;

        void remove_window(
            core::stringid_arg_type name
        ) noexcept;

    private:
        core::allocator& _allocator;
        core::memory::unique_pointer<ImGuiInputs> _imgui_inputs;
        core::memory::unique_pointer<ImGuiRenderer> _imgui_renderer;

        core::pod::Hash<DebugWindow*> _debug_windows;
    };

    class ImGuiModule_DebugSystem : public DebugSystem
    {
    public:
        ImGuiModule_DebugSystem(
            core::allocator& alloc,
            iceshard::Engine& engine
        ) noexcept;
        ~ImGuiModule_DebugSystem() noexcept;

        void register_module(DebugModule& module) noexcept override;

        void unregister_module(DebugModule& module) noexcept override;

        void register_window(
            core::stringid_arg_type name,
            DebugWindow& window
        ) noexcept override;

        void unregister_window(
            core::stringid_arg_type name
        ) noexcept override;

    private:
        core::allocator& _allocator;
        iceshard::Engine& _engine;
        core::memory::unique_pointer<ImGuiDebugSystem> _debug_system;

        union
        {
            ImGuiContext* _imgui_context;
            DebugContextHandle _imgui_handle;
        };
        static_assert(sizeof(_imgui_context) == sizeof(_imgui_handle));
    };

} // namespace debugui::imgui
