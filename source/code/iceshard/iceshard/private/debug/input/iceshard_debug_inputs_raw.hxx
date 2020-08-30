#pragma once
#include <iceshard/debug/debug_window.hxx>

namespace iceshard::debug
{

    class DebugWindow_InputsRaw final : public DebugWindow
    {
    public:
        static constexpr core::stringid_type Identifier = "DebugWindow_InputsRaw"_sid;

        DebugWindow_InputsRaw(core::allocator& alloc, bool& open_ref) noexcept;
        ~DebugWindow_InputsRaw() noexcept override = default;

        void update(Frame const& frame) noexcept override;

        void end_frame() noexcept override;

    private:
        bool& _open;

        core::pod::Queue<iceshard::input::DeviceInputMessage> _messages;
    };

} // namespace iceshard::debug
