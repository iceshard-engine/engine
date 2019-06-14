#include <memsys/allocators/proxy_allocator.h>

#include <cassert>
#include <string>

mem::proxy_allocator::proxy_allocator(const char* name, mem::allocator& alloc)
    : _name{ name }
    , _allocator{ alloc }
    , _total_allocated{ 0 }
{
}

mem::proxy_allocator::~proxy_allocator()
{
    assert(_total_allocated == 0);
}

void* mem::proxy_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/)
{
    void* ptr = _allocator.allocate(size, align);
    _total_allocated += allocated_size(ptr);
    return ptr;
}

void mem::proxy_allocator::deallocate(void* ptr)
{
    if (ptr)
    {
        _total_allocated -= allocated_size(ptr);
        _allocator.deallocate(ptr);
    }
}

uint32_t mem::proxy_allocator::allocated_size(void* ptr)
{
    return _allocator.allocated_size(ptr);
}

uint32_t mem::proxy_allocator::total_allocated()
{
    return _total_allocated;
}

mem::allocator& mem::proxy_allocator::backing_allocator()
{
    return _allocator;
}
