/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct ProxyAllocator final : public ice::Allocator
    {
        inline ProxyAllocator(
            ice::Allocator& backing_allocator,
            std::source_location = std::source_location::current()
        ) noexcept;

        inline ProxyAllocator(
            ice::Allocator& backing_allocator,
            std::string_view name,
            std::source_location = std::source_location::current()
        ) noexcept;

        inline auto backing_allocator() noexcept -> ice::Allocator&;

    protected:
        inline auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        inline void do_deallocate(void* pointer) noexcept override;

    private:
        ice::Allocator& _backing_alloc;
    };

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& backing_allocator,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator }
        , _backing_alloc{ backing_allocator }
    {
    }

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& backing_allocator,
        std::string_view name,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator, name }
        , _backing_alloc{ backing_allocator }
    {
    }

    inline auto ProxyAllocator::backing_allocator() noexcept -> ice::Allocator&
    {
        return _backing_alloc;
    }

    inline auto ProxyAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        return _backing_alloc.allocate(request);
    }

    inline void ProxyAllocator::do_deallocate(void* pointer) noexcept
    {
        _backing_alloc.deallocate(pointer);
    }

} // namespace ice
