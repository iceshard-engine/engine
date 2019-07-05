#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>
#include <input/iolib.h>

#include <core/data/chunk.hxx>
#include <core/datetime/datetime.hxx>
#include <core/debug/assert.hxx>
#include <core/memory.hxx>


namespace input
{

    MessageQueueNew::MessageQueueNew(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _data_queue{ _allocator }
    { }

    MessageQueueNew::~MessageQueueNew() noexcept
    {
        _data_queue.clear();
    }

    void MessageQueueNew::clear() noexcept
    {
        _data_queue.clear();
    }

    auto MessageQueueNew::count() noexcept -> uint32_t
    {
        return _data_queue.count();
    }

    void MessageQueueNew::push(core::cexpr::stringid_argument_type type, core::data_view data) noexcept
    {
        core::data_chunk message_data{ core::memory::globals::default_scratch_allocator(), sizeof(MessageHeader) + data.size() };

        auto* header = reinterpret_cast<MessageHeader*>(message_data.data());
        header->type = type;
        header->timestamp = core::datetime::now().tick;

        auto* data_location = reinterpret_cast<void*>(header + 1);
        memcpy(data_location, data.data(), data.size());

        _data_queue.push(message_data);
    }
}

// Message pipe
input::MessageQueue::MessageQueue(core::allocator& alloc)
    : _allocator{ alloc }
    , _data{ alloc }
{
}

input::MessageQueue::~MessageQueue()
{
    clear();
}

int input::MessageQueue::count() const
{
    return _data.count();
}

void input::MessageQueue::clear()
{
    _data.clear();
}

void input::MessageQueue::for_each(std::function<void(const message::Metadata& metadata, core::data_view)> func) const
{
    std::for_each(_data.begin(), _data.end(), [&](core::data_view message_data)
        {
            auto* metadata = reinterpret_cast<const message::Metadata*>(message_data.data());
            auto message_body = core::data_view{ metadata + 1, message_data.size() - sizeof(message::Metadata) };

            // Call the callback
            func(*metadata, message_body);
        });
}

void input::MessageQueue::push(core::cexpr::stringid_argument_type message_type, core::data_view data)
{
    core::data_chunk chunk{ core::memory::globals::default_scratch_allocator(), sizeof(message::Metadata) + data._size };

    auto* metadata_location = reinterpret_cast<message::Metadata*>(chunk.data());
    metadata_location->message_type = message_type;
    metadata_location->message_timestamp = core::datetime::now().tick;

    auto* data_location = reinterpret_cast<void*>(metadata_location + 1);
    memcpy(data_location, data.data(), data.size());

    _data.push(chunk);
}

void input::filter(const MessageQueue& pipe, const std::vector<MessageFilter>& filters)
{
    pipe.for_each([&filters](const message::Metadata& mdata, core::data_view data)
    {
        for (auto& message_filter : filters)
        {
            if (message_filter.process(mdata, data._data, data._size))
            {
                break; // Don't shame me for this bad code
            }
        }
    });
}
