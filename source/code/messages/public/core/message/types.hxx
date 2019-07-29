#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/datetime/types.hxx>
#include <core/data/view.hxx>

namespace core
{


    //! \brief Defines a simple header for each message.
    struct MessageHeader
    {
        //! \brief The message type.
        core::cexpr::stringid_type type;

        //! \brief The message timestamp.
        core::datetime::tick_type timestamp;
    };

    static_assert(std::is_trivially_copyable_v<MessageHeader>, "The MessageHeader type is required to be trivially copyable!");


    //! \brief Defines a simple header for each message.
    struct Message
    {
        //! \brief The message header.
        core::MessageHeader header;

        //! \brief The message data.
        core::data_view data;
    };

    static_assert(std::is_trivially_copyable_v<Message>, "The Message type is required to be trivially copyable!");


} // namespace core::message
