#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct HostAllocator final : ice::Allocator
    {
        HostAllocator(std::source_location = std::source_location::current()) noexcept;

    protected:
        auto do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result override;
        void do_deallocate(ice::alloc_result result) noexcept override;
    };

} // namespace ice
