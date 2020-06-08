#include "iceshard_debugui.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/keyboard.hxx>

namespace iceshard::debug
{

    IceshardDebugUI::IceshardDebugUI(iceshard::debug::debugui_context_handle ctx) noexcept
        : iceshard::debug::DebugWindow{ ctx }
    {
    }

    void IceshardDebugUI::update(core::MessageBuffer const& messages) noexcept
    {
        using input::KeyboardKey;
        using input::message::KeyboardKeyDown;

        core::message::filter<KeyboardKeyDown>(messages, [&](KeyboardKeyDown const& msg) noexcept
            {
                _visible = _visible ^ (msg.key == KeyboardKey::BackQuote);
            });
    }

    void IceshardDebugUI::end_frame() noexcept
    {
        if (_demo_window)
        {
            ImGui::ShowDemoWindow(&_demo_window);
        }

        if (_visible)
        {
            ImGui::BeginMainMenuBar();
            if (ImGui::BeginMenu("Engine"))
            {
                ImGui::Separator();
                ImGui::MenuItem("Demo window", nullptr, &_demo_window);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

} // namespace iceshard::debug


extern "C"
{
    __declspec(dllexport) auto create_debugui(
        core::allocator& alloc,
        iceshard::debug::debugui_context_handle ctx
    ) -> iceshard::debug::DebugWindow*
    {
        return alloc.make<iceshard::debug::IceshardDebugUI>(ctx);
    }

    __declspec(dllexport) void release_debugui(
        core::allocator& alloc,
        iceshard::debug::DebugWindow* debugui
    )
    {
        alloc.destroy(debugui);
    }
}