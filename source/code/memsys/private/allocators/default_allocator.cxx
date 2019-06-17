#include "default_allocator.hxx"
#include <core/debug/assert.hxx>

namespace memsys
{
namespace detail
{

auto allocation_size_with_header(uint32_t size, uint32_t align) noexcept
{
    return static_cast<uint32_t>(sizeof(memory_tracking::allocation_header) + align + size);
}

} // namespace detail

default_allocator::~default_allocator() noexcept
{
    IS_ASSERT(_total_allocated == 0, "Unreleased memory in default allocator!");
}

auto default_allocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
{
    const uint32_t alloc_size = detail::allocation_size_with_header(size, align);

    auto* alloc_ptr = _aligned_malloc(alloc_size, alignof(memory_tracking::allocation_header));
    auto* alloc_header = reinterpret_cast<memory_tracking::allocation_header*>(alloc_ptr);
    auto* alloc_data = memory_tracking::data_pointer(alloc_header, align);

    memory_tracking::fill(alloc_header, alloc_data, alloc_size, size);
    _total_allocated += alloc_size;
    return alloc_data;
}

void default_allocator::deallocate(void* pointer) noexcept
{
    if (nullptr == pointer)
    {
        return;
    }

    // Release the pointer
    auto* alloc_header = memory_tracking::header(pointer);
    _total_allocated -= alloc_header->allocated_size;
    _aligned_free(alloc_header);
}

auto default_allocator::allocated_size(void* pointer) noexcept -> uint32_t
{
    return memory_tracking::header(pointer)->requested_size;
}

auto default_allocator::total_allocated() noexcept -> uint32_t
{
    return _total_allocated;
}

} // namespace memsys
