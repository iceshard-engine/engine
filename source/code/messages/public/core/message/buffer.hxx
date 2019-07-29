#pragma once
#include <core/allocator.hxx>
#include <core/data/queue.hxx>
#include <core/message/types.hxx>

namespace core
{


    //! \brief This class behaves like a buffer which allows to hold any kind of message.
    class MessageBuffer
    {
    public:
        MessageBuffer(core::allocator& alloc) noexcept;
        ~MessageBuffer() noexcept;

        //! \brief Number of messages in this buffer.
        auto count() const noexcept -> uint32_t;

        //! \brief Inserts a message into the buffer.
        void push(core::cexpr::stringid_argument_type msg) noexcept;

        //! \brief Inserts a message into the buffer.
        void push(core::cexpr::stringid_argument_type msg, core::data_view data) noexcept;

        //! \brief Clears the buffer.
        void clear() noexcept;


    public:
        //! \brief A iterator for the raw data queue.
        class Iterator final
        {
        public:
            Iterator(const MessageBuffer& queue) noexcept;
            Iterator(const MessageBuffer& queue, bool is_end) noexcept;
            ~Iterator() noexcept = default;

            //! \brief Defines equality operators.
            bool operator==(const Iterator& other) noexcept;
            bool operator!=(const Iterator& other) noexcept { return !(*this == other); }

            //! \brief Moves the iterator forward.
            void operator++() noexcept;

            //! \brief Allows to dereference the value.
            auto operator*() const noexcept -> core::Message;

        private:
            //! \brief Additional data.
            core::data_queue::Iterator _iterator;
        };

        //! \brief Begin iterator for this message buffer.
        auto begin() const noexcept -> Iterator;

        //! \brief End iterator for this message buffer.
        auto end() const noexcept -> Iterator;


    private:
        core::data_queue _data_queue;
    };


} // namespace core::message
