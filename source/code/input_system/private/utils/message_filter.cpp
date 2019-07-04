#include <input/utils/message_filter.h>

input::MessageFilter::MessageFilter(core::allocator& alloc)
    : _allocator{ alloc },
    _handle{ nullptr }
{

}

input::MessageFilter::MessageFilter(MessageFilter&& other)
    : _allocator{ other._allocator },
    _handle{ other._handle }
{
    other._handle = nullptr;
}

input::MessageFilter& input::MessageFilter::operator=(MessageFilter&& other)
{
    if (this == &other) return *this;

    MAKE_DELETE(_allocator, IBase, _handle);
    _handle = other._handle;
    other._handle = nullptr;

    return *this;
}

input::MessageFilter::~MessageFilter()
{
    MAKE_DELETE(_allocator, IBase, _handle);
}

bool input::MessageFilter::process(const message::Metadata& mdata, const void* data, uint32_t size) const
{
    if (nullptr == _handle || _handle->message_id() != static_cast<uint64_t>(mdata.identifier.hash_value))
    {
        return false;
    }

    return _handle->process(data, size);
}
