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
        , _dw_inputs_raw{ alloc, _open_inputs_raw }
        , _dw_inputs_states{ alloc, _open_inputs_states }
        , _dw_actions{ alloc, _open_actions }
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
                ImGui::MenuItem("Inputs (raw)", nullptr, &_open_inputs_raw);
                ImGui::MenuItem("Inputs (states)", nullptr, &_open_inputs_states);
                ImGui::MenuItem("Actions", nullptr, &_open_actions);

                ImGui::Separator();
                ImGui::MenuItem("Demo window", nullptr, &_demo_window);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void IceshardDebugUI::register_windows(DebugSystem& system) noexcept
    {
        system.register_window(DebugWindow_InputsRaw::Identifier, _dw_inputs_raw);
        system.register_window(DebugWindow_InputsStates::Identifier, _dw_inputs_states);
        system.register_window(DebugWindow_Actions::Identifier, _dw_actions);
    }

    void IceshardDebugUI::unregister_windows(DebugSystem& system) noexcept
    {
        system.unregister_window(DebugWindow_Actions::Identifier);
        system.unregister_window(DebugWindow_InputsStates::Identifier);
        system.unregister_window(DebugWindow_InputsRaw::Identifier);
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