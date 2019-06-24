#include <core/data/buffer.hxx>
#include <core/debug/assert.hxx>
#include <core/memory.hxx>

#include <cassert>

namespace core
{

Buffer::Buffer(core::allocator& alloc) noexcept
    : _allocator{ &alloc }
{
}

Buffer::Buffer(core::allocator& alloc, data_view data) noexcept
    : _allocator{ &alloc }
{
    buffer::append(*this, data.data(), data.size());
}

Buffer::~Buffer()
{
    _allocator->deallocate(_data);
}

Buffer::Buffer(const Buffer& other) noexcept
    : _allocator{ other._allocator }
{
    const auto other_size = other._size;
    buffer::set_capacity(*this, other_size);
    memcpy(_data, other._data, other_size);
    _size = other_size;
}

auto Buffer::operator=(const Buffer& other) noexcept -> Buffer&
{
    if (this == &other) return *this;

    const uint32_t n = other._size;
    buffer::resize(*this, n);
    memcpy(_data, other._data, n);
    return *this;
}

auto buffer::size(const Buffer& b) noexcept -> uint32_t
{
    return b._size;
}

auto buffer::capacity(const Buffer& b) noexcept -> uint32_t
{
    return b._capacity;
}

bool buffer::empty(const Buffer& b) noexcept
{
    return b._size == 0;
}

auto buffer::data(const Buffer& b) noexcept -> void*
{
    return b._data;
}

void buffer::clear(Buffer& b) noexcept
{
    resize(b, 0);
}

void buffer::trim(Buffer& b) noexcept
{
    set_capacity(b, b._size);
}

void buffer::append(Buffer& b, const void* data, uint32_t size) noexcept
{
    const uint32_t new_size = b._size + size;

    grow(b, new_size);
    IS_ASSERT(new_size <= b._capacity, "Couldn't reserve enough memory for the new buffer size! [ size:{}, capacity:{} ]", new_size, b._capacity);

    void* const buffer_end = core::memory::utils::pointer_add(b._data, b._size);
    memcpy(buffer_end, data, size);

    b._size = new_size;
}

void buffer::resize(Buffer& b, uint32_t new_size) noexcept
{
    if (new_size > b._capacity)
    {
        grow(b, new_size);
    }
    b._size = new_size;
}

void buffer::reserve(Buffer& b, uint32_t new_capacity) noexcept
{
    if (new_capacity > b._capacity)
    {
        set_capacity(b, new_capacity);
    }
}

void buffer::set_capacity(Buffer& b, uint32_t new_capacity) noexcept
{
    if (new_capacity == b._capacity)
    {
        return;
    }

    if (new_capacity < b._size)
    {
        resize(b, new_capacity);
    }

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

void buffer::grow(Buffer& b, uint32_t min_capacity /*= 0*/) noexcept
{
    uint32_t new_capacity = b._capacity * 2 + 8;
    if (new_capacity < min_capacity)
    {
        new_capacity = min_capacity;
    }
    set_capacity(b, new_capacity);
}


} // namespace core
