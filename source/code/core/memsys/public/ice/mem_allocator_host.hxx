/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    class HostAllocator final : public ice::Allocator
    {
    public:
        HostAllocator(std::source_location = std::source_location::current()) noexcept;

    protected:
        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        void do_deallocate(void* pointer) noexcept override;
    };

} // namespace ice
