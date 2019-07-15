#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/data/view.hxx>
#include <functional>

namespace core
{


    struct Message;

    class MessageBuffer;


    //! \brief Contains operations which can be called on a message buffer.
    namespace message
    {

        //! \todo document.
        bool empty(const MessageBuffer& buffer) noexcept;

        //! \todo document.
        auto count(const MessageBuffer& buffer) noexcept -> uint32_t;

        //! \todo document.
        void push(MessageBuffer& buffer, core::cexpr::stringid_argument_type message_type) noexcept;

        //! \todo document.
        void push(MessageBuffer& buffer, core::cexpr::stringid_argument_type message_type, core::data_view message_data) noexcept;

        //! \todo document.
        template<typename T>
        void push(MessageBuffer& buffer, T msg) noexcept;

        //! \todo document.
        void clean(MessageBuffer& buffer) noexcept;

        //! \todo document.
        void for_each(
            const MessageBuffer& buffer
            , std::function<void(const core::Message&)> callback
        ) noexcept;

        //! \todo document.
        void filter(
            const MessageBuffer& buffer
            , const std::vector<core::cexpr::stringid_type>& types
            , std::function<void(const core::Message&)> callback
        ) noexcept;

        //! \todo document.
        void filter(
            const MessageBuffer& buffer
            , core::cexpr::stringid_argument_type type
            , std::function<void(const core::Message&)> callback
        ) noexcept;

        //! \todo document.
        template<typename T>
        void filter(
            const MessageBuffer& buffer
            , std::function<void(const T&)> callback
        ) noexcept;

    } // namespace message


} // namespace core::message
