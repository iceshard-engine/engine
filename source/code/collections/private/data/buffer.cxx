#include <core/data/buffer.hxx>
#include <core/memory.hxx>

#include <cassert>

namespace core
{

Buffer::Buffer(core::allocator& alloc) : _allocator{ &alloc }, _size{ 0u }, _capacity{ 0u }, _data{ nullptr }
{
}

Buffer::Buffer(core::allocator& alloc, void* data, uint32_t size) : _allocator{ &alloc }, _size{ 0u }, _capacity{ 0u }, _data{ nullptr }
{
    buffer::append(*this, data, size);
}

Buffer::~Buffer()
{
    _allocator->deallocate(_data);
}

Buffer::Buffer(const Buffer& other) : _allocator{ other._allocator }, _size{ 0u }, _capacity{ 0u }, _data{ nullptr }
{
    const uint32_t n = other._size;
    buffer::set_capacity(*this, n);
    memcpy(_data, other._data, n);
    _size = n;
}

Buffer& Buffer::operator=(const Buffer& other)
{
    if (this == &other) return *this;

    const uint32_t n = other._size;
    buffer::resize(*this, n);
    memcpy(_data, other._data, n);
    return *this;
}

uint32_t buffer::size(const Buffer& b)
{
    return b._size;
}

uint32_t buffer::capacity(const Buffer& b)
{
    return b._capacity;
}

bool buffer::empty(const Buffer& b)
{
    return b._size == 0;
}

const char* buffer::data(const Buffer& b)
{
    return reinterpret_cast<const char*>(b._data);
}

void buffer::clear(Buffer& b)
{
    resize(b, 0);
}

void buffer::trim(Buffer& b)
{
    set_capacity(b, b._size);
}

void buffer::append(Buffer& b, const void* data, uint32_t size)
{
    const uint32_t n = b._size;
    const uint32_t new_size = n + size;

    reserve(b, new_size);
    assert(new_size <= b._capacity);

    void* buffer_end = core::memory::utils::pointer_add(b._data, n);
    memcpy(buffer_end, data, size);

    b._size = new_size;
}

void buffer::resize(Buffer& b, uint32_t new_size)
{
    if (new_size > b._capacity)
        grow(b, new_size);
    b._size = new_size;
}

void buffer::reserve(Buffer& b, uint32_t new_capacity)
{
    if (new_capacity > b._capacity)
        set_capacity(b, new_capacity);
}

void buffer::set_capacity(Buffer& b, uint32_t new_capacity)
{
    if (new_capacity == b._capacity)
        return;

    if (new_capacity < b._size)
        resize(b, new_capacity);

    void* new_data = nullptr;
    if (new_capacity > 0)
    {
        new_data = b._allocator->allocate(new_capacity);
        memcpy(new_data, b._data, b._size);
    }

    b._allocator->deallocate(b._data);
    b._data = new_data;
    b._capacity = new_capacity;
}

void buffer::grow(Buffer& b, uint32_t min_capacity /*= 0*/)
{
    uint32_t new_capacity = b._capacity * 2 + 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;
    set_capacity(b, new_capacity);
}


} // namespace core
