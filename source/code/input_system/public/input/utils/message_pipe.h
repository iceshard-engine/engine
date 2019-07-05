#pragma once
#include <core/allocator.hxx>
#include <core/pod/hash.hxx>
#include <core/data/queue.hxx>
#include <core/datetime/types.hxx>

#include <input/utils/message_info.h>

#include <functional>
#include <vector>
#include <cassert>

namespace input
{


    //! \brief Defines a single message entry.
    struct MessageHeader
    {
        //! \brief The message type.
        core::cexpr::stringid_type type;

        //! \brief The message timestamp.
        core::datetime::tick_type timestamp;
    };


    //! \brief This class defines a queue for plain data messages.
    class MessageQueueNew final
    {
    public:
        MessageQueueNew(core::allocator& alloc) noexcept;
        ~MessageQueueNew() noexcept;

        //! \brief Clears the queue from all messages.
        void clear() noexcept;

        //! \brief Number of messages in the queue.
        auto count() noexcept -> uint32_t;

        //! \brief Pushes a new message onto the queue.
        void push(core::cexpr::stringid_argument_type type, core::data_view data) noexcept;


    private:
        //! \brief The associated allocator.
        core::allocator& _allocator;

        //! \brief A raw data queue.
        core::data_queue _data_queue;
    };



    class MessageFilter;

    template<class T>
    struct MessageInfo;

    class MessageQueue final
    {
    public:
        MessageQueue(core::allocator& alloc);
        ~MessageQueue();

        MessageQueue(MessageQueue&&) = delete;
        MessageQueue& operator=(MessageQueue&&) = delete;

        MessageQueue(const MessageQueue&) = delete;
        MessageQueue& operator=(const MessageQueue&) = delete;

        int count() const;

        void push(core::cexpr::stringid_argument_type message_type, core::data_view);

        void for_each(std::function<void(const message::Metadata&, core::data_view)> func) const;

        void clear();

    private:
        core::allocator& _allocator;
        core::data_queue _data;
    };

    template<class T>
    void push(MessageQueue& pipe, T&& message);

    template<class T>
    void push(MessageQueue& pipe, const T& message);

    template<class T>
    void for_each(const MessageQueue& pipe, void(*func)(const T&));

    template<class T>
    void for_each(const MessageQueue& pipe, void(*func)(const T&, const message::Metadata&));

    template<class T>
    void for_each(const MessageQueue& pipe, void* userdata, void(*func)(void*, const T&));

    template<class T>
    void for_each(const MessageQueue& pipe, void* userdata, void(*func)(void*, const T&, const message::Metadata&));

    template<class C, class T>
    void for_each(const MessageQueue& pipe, C* obj, void(C::*method)(const T&));

    template<class C, class T>
    void for_each(const MessageQueue& pipe, C* obj, void(C::*method)(const T&, const message::Metadata&));

    void filter(const MessageQueue& pipe, const std::vector<MessageFilter>& filters);

#include "message_pipe.inl"

}
