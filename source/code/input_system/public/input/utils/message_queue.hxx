#pragma once
#include <core/allocator.hxx>
#include <core/data/buffer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/datetime/types.hxx>

#include <functional>

namespace input::message
{


    //! \brief A message metadata object.
    struct Metadata
    {
        //! \brief The message type identifier.
        core::cexpr::stringid_type message_type;

        //! \brief The message timestamp.
        core::datetime::tick_type message_timestamp;
    };


    //! \brief A queue object for messages.
    class Queue final
    {
    public:
        Queue(core::allocator& alloc) noexcept;
        ~Queue() noexcept;

        //! \brief Clears the queue.
        void clear() noexcept;

        //! \brief Pushes a new message with the given metadata and data.
        void push(Metadata metadata, core::data_view data) noexcept;

        //! \brief The current message count.
        auto count() const noexcept -> uint32_t { return _count; }

        //! \brief Visit each entry.
        void visit(std::function<void(const Metadata&, core::data_view data)> callback) const noexcept;

    private:
        core::allocator& _allocator;

        //! \brief Data buffer.
        core::Buffer _data;

        //! \brief Message count.
        uint32_t _count{ 0 };
    };


}
