#pragma once
#include <core/allocator.hxx>

namespace core
{


struct Buffer
{
    Buffer(core::allocator& alloc);
    Buffer(core::allocator& alloc, void* data, uint32_t size);
    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);
    ~Buffer();

    core::allocator* _allocator;
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


} // core
