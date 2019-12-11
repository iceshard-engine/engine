#include <core/message/buffer.hxx>
#include <core/datetime/datetime.hxx>
#include <core/data/chunk.hxx>
#include <core/allocators/stack_allocator.hxx>

namespace core
{

    MessageBuffer::MessageBuffer(core::allocator& alloc) noexcept
        : _data_queue{ alloc }
    {
    }

    MessageBuffer::~MessageBuffer() noexcept
    {
    }

    auto MessageBuffer::count() const noexcept -> uint32_t
    {
        return _data_queue.count();
    }

    void MessageBuffer::push(core::cexpr::stringid_argument_type type) noexcept
    {
        core::data_chunk message_data{ core::memory::globals::default_scratch_allocator(), sizeof(MessageHeader) };

        auto* header = reinterpret_cast<MessageHeader*>(message_data.data());
        header->type = type;
        header->timestamp = core::datetime::now().tick;

        _data_queue.push(message_data);
    }

    void MessageBuffer::push(core::cexpr::stringid_argument_type type, core::data_view_aligned data) noexcept
    {
        core::memory::stack_allocator<core::build::is_release ? 64 : 256> stack_allocator;
        core::data_chunk message_data{ stack_allocator, sizeof(MessageHeader) + data.size() };

        auto* header = reinterpret_cast<MessageHeader*>(message_data.data());
        header->type = type;
        header->timestamp = core::datetime::now().tick;

        auto* data_location = reinterpret_cast<void*>(header + 1);
        std::memcpy(data_location, data.data(), data.size());

        _data_queue.push(message_data);
    }

    void MessageBuffer::clear() noexcept
    {
        _data_queue.clear();
    }

    auto MessageBuffer::begin() const noexcept -> Iterator
    {
        return { *this };
    }

    auto MessageBuffer::end() const noexcept -> Iterator
    {
        return { *this, true };
    }


    //////////////////////////////////////////////////////////////////////////


    MessageBuffer::Iterator::Iterator(const MessageBuffer& queue) noexcept
        : _iterator{ queue._data_queue.begin() }
    {
    }

    MessageBuffer::Iterator::Iterator(const MessageBuffer& queue, bool) noexcept
        : _iterator{ queue._data_queue.end() }
    {
    }

    bool MessageBuffer::Iterator::operator==(const Iterator& other) noexcept
    {
        return _iterator == other._iterator;
    }

    //! \brief Moves the iterator forward.
    void MessageBuffer::Iterator::operator++() noexcept
    {
        ++_iterator;
    }

    //! \brief Allows to dereference the value.
    auto MessageBuffer::Iterator::operator*() const noexcept -> core::Message
    {
        const auto raw_message = (*_iterator);
        const auto* message_header = reinterpret_cast<const MessageHeader*>(raw_message._data);
        const uint32_t message_data_size = raw_message._size - sizeof(MessageHeader);

        return Message{
            *message_header
            , core::data_view{ message_data_size == 0 ? nullptr : message_header + 1, message_data_size }
        };
    }

} // namespace core::message
