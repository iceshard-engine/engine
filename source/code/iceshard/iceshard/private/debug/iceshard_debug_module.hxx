#pragma once
#include <iceshard/debug/debug_window.hxx>
#include "iceshard_debug_input_window.hxx"
#include "iceshard_debug_input_action.hxx"

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

        RawInputsWindow _raw_inputs_window;
        ActionsDebugWindow _actions_window;
    };

} // namespace iceshard::debug

