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
        bool empty(MessageBuffer const& buffer) noexcept;

        //! \todo document.
        auto count(MessageBuffer const& buffer) noexcept -> uint32_t;

        //! \todo document.
        void push(MessageBuffer& buffer, core::stringid_arg_type message_type) noexcept;

        //! \todo document.
        void push(MessageBuffer& buffer, core::stringid_arg_type message_type, core::data_view_aligned data) noexcept;

        //! \todo document.
        template<typename T>
        void push(MessageBuffer& buffer, T const& msg) noexcept;

        //! \todo document.
        void clear(MessageBuffer& buffer) noexcept;

        //! \todo document.
        void for_each(
            MessageBuffer const& buffer
            , std::function<void(core::Message const&)> callback
        ) noexcept;

        //! \todo document.
        void filter(
            MessageBuffer const& buffer
            , std::vector<core::stringid_type> const& types
            , std::function<void(core::Message const&)> callback
        ) noexcept;

        //! \todo document.
        void filter(
            MessageBuffer const& buffer
            , core::stringid_arg_type type
            , std::function<void(core::Message const&)> callback
        ) noexcept;

        //! \todo document.
        template<typename T>
        void filter(
            MessageBuffer const& buffer
            , std::function<void(T const&)> callback
        ) noexcept;

    } // namespace message


    namespace detail
    {

        template <typename T, typename = int>
        struct has_type_member : std::false_type { };

        template <typename T>
        struct has_type_member <T, decltype((void)T::message_type, 0)> : std::true_type { };

    } // namespace detail


    template<typename T>
    void message::push(MessageBuffer& buffer, const T& msg) noexcept
    {
        static_assert(
            std::is_trivially_copyable_v<T>
            , "Message object not trivially copyable!"
        );
        static_assert(
            detail::has_type_member<T>::value
            , "Message missing static member 'message_type'!"
        );
        static_assert(
            std::is_same_v<std::remove_cv_t<decltype(T::message_type)>, core::stringid_type>
            , "Invalid type of message member 'message_type', expected 'core::cexpr::stringid_type'!"
        );

        message::push(buffer, T::message_type, { &msg, sizeof(T), alignof(T) });
    }

    template<typename T>
    void message::filter(const MessageBuffer& buffer, std::function<void(const T&)> callback) noexcept
    {
        static_assert(
            std::is_trivially_copyable_v<T>
            , "Message object not trivially copyable!"
        );
        static_assert(
            detail::has_type_member<T>::value
            , "Message missing static member 'message_type'!"
        );
        static_assert(
            std::is_same_v<std::remove_cv_t<decltype(T::message_type)>, core::stringid_type>
            , "Invalid type of message member 'message_type', expected 'core::cexpr::stringid_type'!"
        );

        message::filter(buffer, T::message_type, [&](const core::Message& msg) noexcept
            {
                callback(*reinterpret_cast<const T*>(msg.data.data()));
            });
    }


} // namespace core::message
