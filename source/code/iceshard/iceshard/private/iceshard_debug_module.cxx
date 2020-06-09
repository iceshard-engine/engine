#include "iceshard_debug_module.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/keyboard.hxx>
#include <iceshard/debug/debug_module.hxx>
#include <iceshard/debug/debug_system.hxx>

namespace iceshard::debug
{

    namespace detail
    {

        class Ice_EngineDebugModule : public DebugModule
        {
        public:
            void on_initialize(DebugSystem& system) noexcept override
            {
                system.register_window("iceshard-window"_sid, _debugui);
            }

            void on_deinitialize(DebugSystem& system) noexcept override
            {
                system.unregister_window("iceshard-window"_sid);
            }

        private:
            IceshardDebugUI _debugui;
        };

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
    __declspec(dllexport) auto create_debug_module(
        core::allocator& alloc,
        iceshard::debug::DebugSystem& debug_system
    ) -> iceshard::debug::DebugModule*
    {
        return alloc.make<iceshard::debug::detail::Ice_EngineDebugModule>();
    }

    __declspec(dllexport) void release_debug_module(
        core::allocator& alloc,
        iceshard::debug::DebugModule* debug_module
    )
    {
        alloc.destroy(debug_module);
    }
}