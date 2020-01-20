#include "iceshard_debugui.hxx"

namespace iceshard::debug
{

    IceshardDebugUI::IceshardDebugUI(debugui::debugui_context_handle ctx) noexcept
        : debugui::DebugUI{ ctx }
    {
    }

    void IceshardDebugUI::end_frame() noexcept
    {
        ImGui::BeginMainMenuBar();
        ImGui::EndMainMenuBar();
    }

} // namespace iceshard::debug


extern "C"
{
    __declspec(dllexport) auto create_debugui(core::allocator& alloc, debugui::debugui_context_handle ctx) -> debugui::DebugUI*
    {
        return alloc.make<iceshard::debug::IceshardDebugUI>(ctx);
    }

    __declspec(dllexport) void release_debugui(core::allocator& alloc, debugui::DebugUI* debugui)
    {
        alloc.destroy(debugui);
    }
}