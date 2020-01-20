#pragma once
#include <core/pointer.hxx>
#include <debugui/debugui_module.hxx>
#include "imgui_inputs.hxx"
#include "imgui_renderer.hxx"

#include <imgui/imgui.h>

namespace debugui::imgui
{

    class DebugUIContext_ImGui : public DebugUIContext
    {
    public:
        DebugUIContext_ImGui(core::allocator& alloc, input::InputSystem& input_system, asset::AssetSystem& asset_system, render::RenderSystem& render_system) noexcept;
        ~DebugUIContext_ImGui() noexcept;

        void update(core::MessageBuffer const& messages) noexcept override;

        void begin_frame() noexcept override;

        void end_frame() noexcept override;

        auto context_handle() const noexcept -> debugui_context_handle override
        {
            return _imgui_handle;
        }

    private:
        core::allocator& _allocator;
        core::memory::unique_pointer<ImGuiInputs> _imgui_inputs;
        core::memory::unique_pointer<ImGuiRenderer> _imgui_renderer;

        union
        {
            ImGuiContext* _imgui_context;
            debugui_context_handle _imgui_handle;
        };
        static_assert(sizeof(_imgui_context) == sizeof(_imgui_handle));
    };

} // namespace debugui::imgui
