#pragma once
#include <iceshard/debug/debug_window.hxx>
#include <core/pod/collections.hxx>

#include "../../systems/iceshard_input_actions.hxx"

namespace iceshard::debug
{

    class DebugWindow_Actions final : public DebugWindow
    {
    public:
        static constexpr core::stringid_type Identifier = "DebugWindow_Actions"_sid;

        DebugWindow_Actions(core::allocator& alloc, bool& open_ref) noexcept;

        void update(iceshard::Frame const& frame) noexcept override;

        void end_frame() noexcept override;

    private:
        bool& _open;

        core::pod::Array<InputAction const*> _actions;
        core::pod::Hash<InputActionState const*> _action_states;
    };

} // namespace iceshard::debug
