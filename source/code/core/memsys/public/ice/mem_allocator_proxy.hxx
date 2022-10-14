#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct ProxyAllocator final : ice::Allocator
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

    protected:
        inline auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        inline void do_deallocate(ice::Memory memory) noexcept override;

    private:
        ice::Allocator& _backing_alloc;
    };

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& backing_alloc,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_alloc }
        , _backing_alloc{ backing_alloc }
    {
    }

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& backing_alloc,
        std::string_view name,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_alloc, name }
        , _backing_alloc{ backing_alloc }
    {
    }

    inline auto ProxyAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        return _backing_alloc.allocate(request);
    }

    inline void ProxyAllocator::do_deallocate(ice::Memory memory) noexcept
    {
        _backing_alloc.deallocate(memory);
    }

} // namespace ice
