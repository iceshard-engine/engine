#pragma once
#include <core/message/buffer.hxx>
#include <iceshard/input/device/input_device_queue.hxx>

namespace input
{

    //! \brief Describes a media driver, which is responsible to handle low-level media events and return proper messages.
    class InputSystem
    {
    public:
        virtual ~InputSystem() noexcept = default;

        //! \brief Queries the media driver for messages.
        [[deprecated]]
        virtual void query_messages(core::MessageBuffer& message_buffer) const noexcept = 0;

        //! \brief Queries for device input events.
        virtual void query_events(
            core::MessageBuffer& message_buffer,
            iceshard::input::DeviceInputQueue& queue
        ) noexcept = 0;
    };

} // namespace input
