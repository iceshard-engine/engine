#pragma once
#include <core/allocator.hxx>
#include <core/message/types.hxx>
#include <core/pointer.hxx>

#include <iceshard/input/device/input_device_queue.hxx>

namespace iceshard::debug
{

    class DebugWindow
    {
    public:
        virtual ~DebugWindow() noexcept = default;

        virtual void update(iceshard::input::DeviceInputQueue const& inputs) noexcept { }

        virtual void begin_frame() noexcept { }

        virtual void end_frame() noexcept { }
    };

} // namespace debugui
