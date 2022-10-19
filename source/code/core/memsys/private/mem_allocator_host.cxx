#include <ice/mem_allocator_host.hxx>

namespace ice
{

    HostAllocator::HostAllocator(std::source_location src_loc) noexcept
        : Allocator{ src_loc }
    {
    }

    auto HostAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        return ice::alloc_aligned(request.size, request.alignment);
    }

    void HostAllocator::do_deallocate(void* pointer) noexcept
    {
        ice::release_aligned(pointer);
    }

} // namespace ice
