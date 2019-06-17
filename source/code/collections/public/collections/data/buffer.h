#pragma once
#include <memsys/allocator.hxx>

namespace pod
{

class data_ptr final
{
public:
    using data_t = void*;
    using size_t = std::size_t;

    explicit data_ptr(memsys::allocator& alloc, data_t ptr, size_t sz);
    ~data_ptr();

    constexpr data_ptr(const data_ptr&) noexcept = delete;
    constexpr data_ptr& operator=(const data_ptr&) noexcept = delete;

    constexpr data_ptr(data_ptr&&) noexcept = delete;
    constexpr data_ptr& operator=(data_ptr&&) noexcept = delete;

    size_t size() const noexcept { return _size; }

    data_t data() const noexcept { return _data; }

private:
    memsys::allocator& _allocator;
    const data_t _data;
    const size_t _size;
};

struct Buffer
{
    Buffer(memsys::allocator& alloc);
    Buffer(memsys::allocator& alloc, void* data, uint32_t size);
    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);
    ~Buffer();

    memsys::allocator* _allocator;
    uint32_t _size;
    uint32_t _capacity;
    void* _data;
};

namespace buffer
{

//! Returns the sized used by this given buffer
uint32_t size(const Buffer& b);

//! Returns the capacity of the given buffer
uint32_t capacity(const Buffer& b);

//! Returns true if the buffer is empty
bool empty(const Buffer& b);

//! Returns the underlying data buffer
const char* data(const Buffer& b);

//! Appends data to the buffer
void append(Buffer& b, const void* data, uint32_t size);

//! Changes the size of the buffer (does not reallocate memory unless necessary).
void resize(Buffer& a, uint32_t new_size);

//! Removes all items in the buffer (does not free memory).
void clear(Buffer& b);

//! Reallocates the buffer to the specified capacity.
void set_capacity(Buffer& b, uint32_t new_capacity);

//! Grows the buffer using a geometric progression formula
//! If a min_capacity is specified, the buffer will grow
//! to at least that capacity.
void grow(Buffer& b, uint32_t min_capacity = 0);

//! Makes sure that the buffer has at least the specified capacity.
//! (If not, the buffer is grown.)
void reserve(Buffer& b, uint32_t new_capacity);

//! Trims the buffer so that its capacity matches its size.
void trim(Buffer& b);

}

}
