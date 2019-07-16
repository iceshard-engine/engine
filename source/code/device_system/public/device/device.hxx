#pragma once
#include <core/string_types.hxx>
#include <core/message/buffer.hxx>

namespace device
{


    //! \brief Supported device types
    enum DeviceType
    {
        None
        , Mouse
        , Keyboard
        , GamePad
        , Display
    };


    //! \brief A interface representing a single device.
    class Device
    {
    public:
        virtual ~Device() noexcept = default;

        //! \brief Device type.
        virtual auto type() const noexcept -> DeviceType = 0;

        //! \brief Device name.
        virtual auto name() const noexcept -> core::StringView<> = 0;

        //! \brief Updates the device and prepares the internal message queue.
        virtual void update() noexcept = 0;

        //! \brief Queries the device for messages.
        virtual void query(core::MessageBuffer& message_buffer) const noexcept = 0;
    };


} // namespace input
