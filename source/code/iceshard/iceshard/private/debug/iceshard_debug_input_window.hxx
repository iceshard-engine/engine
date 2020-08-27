#pragma once
#include <iceshard/debug/debug_window.hxx>
#include <iceshard/input/device/input_device_message.hxx>

#include <core/pod/collections.hxx>
#include <core/clock.hxx>

namespace iceshard::debug
{

    class RawInputsWindow : public DebugWindow
    {
    public:
        RawInputsWindow(core::allocator& alloc) noexcept;
        ~RawInputsWindow() noexcept override;

        void update(iceshard::Frame const& frame) noexcept override;

        void end_frame() noexcept override;

        void show() noexcept;

    private:
        core::allocator& _allocator;
        bool _visible[2] = { false, false };

        uint64_t _current_frame = 0;

        struct InputEventEx
        {
            core::Timeline last_update;
            iceshard::input::InputEvent input;
        };

        core::pod::Queue<iceshard::input::DeviceInputMessage> _messages;
        core::pod::Hash<InputEventEx> _events;
    };

} // namespace iceshard::debug
