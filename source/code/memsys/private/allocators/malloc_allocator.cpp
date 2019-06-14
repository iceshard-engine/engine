#pragma once
#include <memsys/allocators/malloc_allocator.h>
#include <cassert>

namespace mem
{

malloc_allocator::malloc_allocator()
{ }

malloc_allocator::~malloc_allocator()
{ }

void* malloc_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/)
{
    return _aligned_malloc(size, align);
}

void malloc_allocator::deallocate(void* ptr)
{
    _aligned_free(ptr);
}

uint32_t malloc_allocator::allocated_size(void* ptr)
{
    return SIZE_NOT_TRACKED;
}

uint32_t malloc_allocator::total_allocated()
{
    return SIZE_NOT_TRACKED; // #todo track at least the total allocated size so we know if somethig leaked...
}

}
