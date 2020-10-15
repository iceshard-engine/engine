#include "default_allocator.hxx"
#include <cassert>

namespace ice::memory
{
    namespace detail
    {

        auto allocation_size_with_header(uint32_t size, uint32_t align) noexcept
        {
            return static_cast<uint32_t>(sizeof(tracking::AllocationHeader) + align + size);
        }

    } // namespace detail

    DefaultAllocator::~DefaultAllocator() noexcept
    {
        assert(_total_allocated == 0); // , "Unreleased memory in default allocator!");
    }

    auto DefaultAllocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        uint32_t const alloc_size = detail::allocation_size_with_header(size, align);

        auto* alloc_ptr = _aligned_malloc(alloc_size, alignof(tracking::AllocationHeader));
        auto* alloc_header = reinterpret_cast<tracking::AllocationHeader*>(alloc_ptr);
        auto* alloc_data = tracking::data_pointer(alloc_header, align);

        tracking::fill(alloc_header, alloc_data, alloc_size, size);
        _total_allocated += alloc_size;
        return alloc_data;
    }

    void DefaultAllocator::deallocate(void* pointer) noexcept
    {
        if (nullptr == pointer)
        {
            return;
        }

        // Release the pointer
        auto* alloc_header = tracking::header(pointer);
        _total_allocated -= alloc_header->allocated_size;
        _aligned_free(alloc_header);
    }

    auto DefaultAllocator::allocated_size(void* pointer) noexcept -> uint32_t
    {
        return tracking::header(pointer)->requested_size;
    }

    auto DefaultAllocator::total_allocated() noexcept -> uint32_t
    {
        return _total_allocated;
    }

} // namespace ice::memory
