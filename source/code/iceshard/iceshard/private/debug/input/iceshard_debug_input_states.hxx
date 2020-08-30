#pragma once
#include <iceshard/debug/debug_window.hxx>

namespace iceshard::debug
{

    class DebugWindow_InputsStates final : public DebugWindow
    {
    public:
        static constexpr core::stringid_type Identifier = "DebugWindow_InputsStates"_sid;

        DebugWindow_InputsStates(core::allocator& alloc, bool& open_ref) noexcept;
        ~DebugWindow_InputsStates() noexcept override = default;

        void update(Frame const& frame) noexcept override;

        void end_frame() noexcept override;

    private:
        bool& _open;

        struct InputEventEx
        {
            core::Timeline last_update;
            iceshard::input::InputEvent input;
        };

        core::pod::Hash<InputEventEx> _events;
    };

} // namespace iceshard::debug
