#include <memsys/allocators/proxy_allocator.h>
#include <cassert>
#include <string>


namespace memsys
{


proxy_allocator::proxy_allocator(const char* name, allocator& alloc) noexcept
    : _name{ name }
    , _allocator{ alloc }
    , _total_allocated{ 0 }
{
}

proxy_allocator::~proxy_allocator() noexcept
{
    assert(_total_allocated == 0);
}

void* proxy_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept
{
    void* ptr = _allocator.allocate(size, align);
    _total_allocated += allocated_size(ptr);
    return ptr;
}

void proxy_allocator::deallocate(void* ptr) noexcept
{
    if (ptr)
    {
        _total_allocated -= allocated_size(ptr);
        _allocator.deallocate(ptr);
    }
}

uint32_t proxy_allocator::allocated_size(void* ptr) noexcept
{
    return _allocator.allocated_size(ptr);
}

uint32_t proxy_allocator::total_allocated() noexcept
{
    return _total_allocated;
}

allocator& proxy_allocator::backing_allocator() noexcept
{
    return _allocator;
}

} // namespace memsys
