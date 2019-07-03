#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>
#include <input/iolib.h>

#include <core/debug/assert.hxx>

// Message pipe
mooned::io::MessagePipe::MessagePipe(core::allocator& alloc)
    : _allocator{ alloc }
    , _data{ alloc }
{
}

mooned::io::MessagePipe::~MessagePipe()
{
    clear();
}

int mooned::io::MessagePipe::count() const
{
    return _data.size();
}

void mooned::io::MessagePipe::clear()
{
    _data.clear();
}

void mooned::io::MessagePipe::for_each(std::function<void(const message::Metadata& metadata, const void* data, int size)> func) const
{
    _data.for_each(func);
}

void mooned::io::MessagePipe::push(uint64_t identifier, const void* data, int size)
{
    _data.push(
        message::Metadata{
            core::cexpr::stringid_type{
                core::cexpr::stringid_hash_type{ identifier }
            }
            , mooned::io::ticks()
        }
        , data
        , size
    );
}

void mooned::io::filter(const MessagePipe& pipe, const std::vector<MessageFilter>& filters)
{
    pipe.for_each([&filters](const message::Metadata& mdata, const void* data, int size)
    {
        for (auto& message_filter : filters)
        {
            if (message_filter.process(mdata, data, size))
            {
                break; // Don't shame me for this bad code
            }
        }
    });
}
