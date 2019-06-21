#include <core/allocators/proxy_allocator.hxx>
#include <core/debug/assert.hxx>

namespace core::memory
{

proxy_allocator::proxy_allocator(std::string_view name, core::allocator& alloc) noexcept
    : _name{ std::move(name) }
    , _backing_allocator{ alloc }
    , _allocation_tracking{ _backing_allocator.total_allocated() != SIZE_NOT_TRACKED }
{
}

proxy_allocator::~proxy_allocator() noexcept
{
    if (_allocation_tracking)
    {
        IS_ASSERT(_allocation_total == 0, "Allocated memory was not released in allocator {}! [ not released: {}b ]"
            , _name, _allocation_total
        );
    }
}

auto proxy_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept -> void*
{
    _allocation_requests += 1;

    void* ptr = _backing_allocator.allocate(size, align);
    if (_allocation_tracking)
    {
        _allocation_total += allocated_size(ptr);
    }
    return ptr;
}

void proxy_allocator::deallocate(void* ptr) noexcept
{
    if (ptr)
    {
        if (_allocation_tracking)
        {
            _allocation_total -= allocated_size(ptr);
        }
        _backing_allocator.deallocate(ptr);
    }
}

auto proxy_allocator::allocated_size(void* ptr) noexcept -> uint32_t
{
    return _backing_allocator.allocated_size(ptr);
}

auto proxy_allocator::total_allocated() noexcept -> uint32_t
{
    return _allocation_tracking ? _allocation_total : SIZE_NOT_TRACKED;
}

} // namespace core::memory
