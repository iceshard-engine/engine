#include <ice/memory/proxy_allocator.hxx>
#include <ice/profiler.hxx>
#include <cassert>

namespace ice::memory
{

#if ICE_DEBUG || ICE_DEVELOP

    ProxyAllocator::ProxyAllocator(ice::Allocator& alloc, std::string_view name) noexcept
        : ice::Allocator{ alloc, name }
        , _backing_allocator{ alloc }
        , _allocation_tracking{ _backing_allocator.total_allocated() != Constant_SizeNotTracked }
    {
    }

    ProxyAllocator::~ProxyAllocator() noexcept
    {
        if (_allocation_tracking)
        {
            assert(_allocation_total == 0);/* , "Allocated memory was not released in allocator {}! [ not released: {}b ]"
                , _name, _allocation_total
            );*/
        }
    }

    auto ProxyAllocator::allocate(uint32_t size, uint32_t align /*= Constant_DefaultAlignment*/) noexcept -> void*
    {
        _allocation_requests += 1;

        void* ptr = _backing_allocator.allocate(size, align);
        if (_allocation_tracking)
        {
            _allocation_total += allocated_size(ptr);
        }
        return ptr;
    }

    void ProxyAllocator::deallocate(void* ptr) noexcept
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

    auto ProxyAllocator::allocated_size(void* ptr) const noexcept -> uint32_t
    {
        return _backing_allocator.allocated_size(ptr);
    }

    auto ProxyAllocator::total_allocated() const noexcept -> uint32_t
    {
        return _allocation_tracking ? _allocation_total : Constant_SizeNotTracked;
    }

#elif ICE_PROFILE

    auto ProxyAllocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        void* ptr = _backing_allocator.allocate(size, align);
        // TracyAllocNS(ptr, size, 8, _name.data());
        return ptr;
    }

    void ProxyAllocator::deallocate(void* ptr) noexcept
    {
        // TracyFreeNS(ptr, 8, _name.data());
        return _backing_allocator.deallocate(ptr);
    }

#endif // #if ICE_DEBUG || ICE_DEVELOP

} // namespace ice::memory
