#include "imgui_context.hxx"
#include <core/allocator.hxx>
#include <input_system/keyboard.hxx>

namespace debugui::imgui
{

    namespace detail
    {

        auto create_imgui_context() noexcept -> ImGuiContext*
        {
            auto* current_context = ImGui::GetCurrentContext();
            if (nullptr == current_context)
            {
                current_context = ImGui::CreateContext();
            }
            return current_context;
        }

        void release_debugui_context(ImGuiContext* context) noexcept
        {
            ImGui::DestroyContext(context);
        }

    } // namespace detail

    DebugUIContext_ImGui::DebugUIContext_ImGui(core::allocator& alloc, input::InputSystem& input_system, asset::AssetSystem& asset_system, render::RenderSystem& render_system) noexcept
        : _allocator{ alloc }
        , _imgui_context{ detail::create_imgui_context() }
        , _imgui_inputs{ nullptr, { _allocator } }
        , _imgui_renderer{ nullptr, { _allocator } }
    {
        _imgui_inputs = core::memory::make_unique<ImGuiInputs>(_allocator, ImGui::GetIO(), input_system);
        _imgui_renderer = core::memory::make_unique<ImGuiRenderer>(_allocator, _allocator, ImGui::GetIO(), asset_system, render_system);
    }

    DebugUIContext_ImGui::~DebugUIContext_ImGui() noexcept
    {
        _imgui_renderer = nullptr;
        _imgui_inputs = nullptr;
        detail::release_debugui_context(_imgui_context);
    }

    void DebugUIContext_ImGui::update(core::MessageBuffer const& messages) noexcept
    {
        _imgui_inputs->update(messages);
    }

    void DebugUIContext_ImGui::begin_frame() noexcept
    {
        ImGui::NewFrame();
    }

    void DebugUIContext_ImGui::end_frame() noexcept
    {
        ImGui::EndFrame();
        ImGui::Render();
        _imgui_renderer->Draw(ImGui::GetDrawData());
    }

} // namespace debugui::imgui

extern "C"
{
    __declspec(dllexport) auto create_debugui(core::allocator& alloc, input::InputSystem& input_system, asset::AssetSystem& asset_system, render::RenderSystem& render_system) -> debugui::imgui::DebugUIContext_ImGui*
    {
        return alloc.make<debugui::imgui::DebugUIContext_ImGui>(alloc, input_system, asset_system, render_system);
    }

    __declspec(dllexport) void release_debugui(core::allocator& alloc, debugui::imgui::DebugUIContext_ImGui* driver)
    {
         alloc.destroy(driver);
    }
}
