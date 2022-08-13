#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct HostAllocator final : ice::Allocator
    {
        HostAllocator(std::source_location = std::source_location::current()) noexcept;

    protected:
        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        void do_deallocate(ice::Memory result) noexcept override;
    };

} // namespace ice
