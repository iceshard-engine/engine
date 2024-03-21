/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_host.hxx>

namespace ice
{

    HostAllocator::HostAllocator(std::source_location src_loc) noexcept
        : Allocator{ src_loc, "Host" }
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
