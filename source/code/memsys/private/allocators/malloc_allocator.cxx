#pragma once
#include <memsys/allocators/malloc_allocator.hxx>
#include <cassert>


namespace memsys
{

malloc_allocator::malloc_allocator() noexcept
{ }

malloc_allocator::~malloc_allocator() noexcept
{ }

void* malloc_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept
{
    return _aligned_malloc(size, align);
}

void malloc_allocator::deallocate(void* ptr) noexcept
{
    _aligned_free(ptr);
}

uint32_t malloc_allocator::allocated_size(void*) noexcept
{
    return SIZE_NOT_TRACKED;
}

uint32_t malloc_allocator::total_allocated() noexcept
{
    return SIZE_NOT_TRACKED; // #todo track at least the total allocated size so we know if somethig leaked...
}


} // namespace memsys
