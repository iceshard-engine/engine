#pragma once
#include <core/message/buffer.hxx>

namespace input
{


    //! \brief Describes a media driver, which is responsible to handle low-level media events and return proper messages.
    class InputSystem
    {
    public:
        virtual ~InputSystem() noexcept = default;

        //! \brief Queries the media driver for messages.
        virtual void query_messages(core::MessageBuffer& message_buffer) const noexcept = 0;
    };


} // namespace input