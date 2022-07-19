#include <ice/mem_allocator_host.hxx>

namespace ice
{

    HostAllocator::HostAllocator(std::source_location src_loc) noexcept
        : Allocator{ src_loc }
    {
    }

    auto HostAllocator::do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result
    {
        return ice::alloc_aligned(request);
    }

    void HostAllocator::do_deallocate(ice::alloc_result result) noexcept
    {
        ice::release_aligned(result);
    }

} // namespace ice
