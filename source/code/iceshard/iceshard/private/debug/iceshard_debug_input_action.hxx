#pragma once
#include <iceshard/debug/debug_window.hxx>
#include <core/pod/collections.hxx>

#include "../systems/iceshard_input_actions.hxx"

namespace iceshard::debug
{

    class ActionsDebugWindow : public DebugWindow
    {
    public:
        ActionsDebugWindow(core::allocator& alloc) noexcept;


        void update(iceshard::Frame const& frame) noexcept override;

        void end_frame() noexcept override;

        void show() noexcept;

    private:
        bool _visible = false;

        core::pod::Array<InputAction const*> _actions;
        core::pod::Hash<InputActionState const*> _action_states;
    };

} // namespace iceshard::debug
