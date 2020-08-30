#pragma once
#include <iceshard/debug/debug_window.hxx>

#include "input/iceshard_debug_inputs_raw.hxx"
#include "input/iceshard_debug_input_states.hxx"
#include "input/iceshard_debug_actions.hxx"

namespace iceshard::debug
{

    class DebugSystem;

    class IceshardDebugUI : public iceshard::debug::DebugWindow
    {
    public:
        IceshardDebugUI(core::allocator& alloc) noexcept;

        void update(iceshard::Frame const& frame) noexcept override;

        void end_frame() noexcept override;

        void register_windows(DebugSystem& debug_system) noexcept;
        void unregister_windows(DebugSystem& debug_system) noexcept;

    private:
        bool _visible = false;
        bool _demo_window = false;

        // Windows states (in order)
        bool _open_inputs_raw = false;
        bool _open_inputs_states = false;
        bool _open_actions = false;

        // Windows (in order)
        DebugWindow_InputsRaw _dw_inputs_raw;
        DebugWindow_InputsStates _dw_inputs_states;
        DebugWindow_Actions _dw_actions;
    };

} // namespace iceshard::debug

