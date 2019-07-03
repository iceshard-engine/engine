#include <iolib/utils/message_filter.h>

mooned::io::MessageFilter::MessageFilter(mem::allocator& alloc)
    : _allocator{ alloc },
    _handle{ nullptr }
{

}

mooned::io::MessageFilter::MessageFilter(MessageFilter&& other)
    : _allocator{ other._allocator },
    _handle{ other._handle }
{
    other._handle = nullptr;
}

mooned::io::MessageFilter& mooned::io::MessageFilter::operator=(MessageFilter&& other)
{
    if (this == &other) return *this;

    MAKE_DELETE(_allocator, IBase, _handle);
    _handle = other._handle;
    other._handle = nullptr;

    return *this;
}

mooned::io::MessageFilter::~MessageFilter()
{
    MAKE_DELETE(_allocator, IBase, _handle);
}

bool mooned::io::MessageFilter::process(const message::Metadata& mdata, const void* data, uint32_t size) const
{
    if (nullptr == _handle || _handle->message_id() != mdata.identifier)
    {
        return false;
    }

    return _handle->process(data, size);
}
