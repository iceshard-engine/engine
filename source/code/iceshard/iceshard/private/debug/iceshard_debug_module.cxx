#include "iceshard_debug_module.hxx"

#include <core/message/operations.hxx>
#include <core/pod/algorithm.hxx>

#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/device/input_device_queue.hxx>

#include <iceshard/debug/debug_module.hxx>
#include <iceshard/debug/debug_system.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug
{

    namespace detail
    {

        class Ice_EngineDebugModule : public DebugModule
        {
        public:
            Ice_EngineDebugModule(core::allocator& alloc) noexcept
                : _debugui{ alloc }
            {
            }

            void on_initialize(DebugSystem& system) noexcept override
            {
                system.register_window("iceshard-window"_sid, _debugui);
                _debugui.register_windows(system);
            }

            void on_deinitialize(DebugSystem& system) noexcept override
            {
                _debugui.unregister_windows(system);
                system.unregister_window("iceshard-window"_sid);
            }

        private:
            IceshardDebugUI _debugui;
        };

    }

    IceshardDebugUI::IceshardDebugUI(core::allocator& alloc) noexcept
        : DebugWindow{ }
        , _raw_inputs_window{ alloc }
        , _actions_window{ alloc }
    {
    }

    void IceshardDebugUI::update(iceshard::Frame const& frame) noexcept
    {
        using namespace iceshard::input;

        for (auto const& input : frame.input_events())
        {
            if (input.identifier == create_inputid(DeviceType::Keyboard, KeyboardKey::BackQuote))
            {
                _visible = _visible ^ (input.value.button.state.clicked || input.value.button.state.repeat);
            }
        }
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
                if (ImGui::MenuItem("Input (raw)"))
                {
                    _raw_inputs_window.show();
                }
                if (ImGui::MenuItem("Actions"))
                {
                    _actions_window.show();
                }
                ImGui::Separator();
                ImGui::MenuItem("Demo window", nullptr, &_demo_window);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void IceshardDebugUI::register_windows(DebugSystem& system) noexcept
    {
        system.register_window("iceshard.raw-inputs"_sid, _raw_inputs_window);
        system.register_window("iceshard.debug.actions"_sid, _actions_window);
    }

    void IceshardDebugUI::unregister_windows(DebugSystem& system) noexcept
    {
        system.unregister_window("iceshard.debug.actions"_sid);
        system.unregister_window("iceshard.raw-inputs"_sid);
    }

} // namespace iceshard::debug


extern "C"
{
    __declspec(dllexport) auto create_debug_module(
        core::allocator& alloc,
        iceshard::debug::DebugSystem& debug_system
    ) -> iceshard::debug::DebugModule*
    {
        return alloc.make<iceshard::debug::detail::Ice_EngineDebugModule>(alloc);
    }

    __declspec(dllexport) void release_debug_module(
        core::allocator& alloc,
        iceshard::debug::DebugModule* debug_module
    )
    {
        alloc.destroy(debug_module);
    }
}