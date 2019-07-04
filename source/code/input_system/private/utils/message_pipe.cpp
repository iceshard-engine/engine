#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>
#include <input/iolib.h>

#include <core/datetime/datetime.hxx>
#include <core/debug/assert.hxx>

// Message pipe
input::MessagePipe::MessagePipe(core::allocator& alloc)
    : _allocator{ alloc }
    , _data{ alloc }
{
}

input::MessagePipe::~MessagePipe()
{
    clear();
}

int input::MessagePipe::count() const
{
    return _data.count();
}

void input::MessagePipe::clear()
{
    _data.clear();
}

void input::MessagePipe::for_each(std::function<void(const message::Metadata& metadata, core::data_view)> func) const
{
    _data.visit(func);
}

void input::MessagePipe::push(core::cexpr::stringid_argument_type message_type, core::data_view data)
{
    _data.push(message::Metadata{ message_type, core::datetime::now().tick }, data);
}

void input::filter(const MessagePipe& pipe, const std::vector<MessageFilter>& filters)
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
