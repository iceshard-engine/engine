#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct ProxyAllocator final : ice::Allocator
    {
        inline ProxyAllocator(
            ice::Allocator& parent,
            std::source_location = std::source_location::current()
        ) noexcept;

        inline ProxyAllocator(
            ice::Allocator& parent,
            std::u8string_view name,
            std::source_location = std::source_location::current()
        ) noexcept;

    private:
        inline auto do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result override;
        inline void do_deallocate(ice::alloc_result result) noexcept override;

    private:
        ice::Allocator& _parent;
    };

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& parent,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, parent }
        , _parent{ parent }
    {
    }

    inline ProxyAllocator::ProxyAllocator(
        ice::Allocator& parent,
        std::u8string_view name,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, parent, name }
        , _parent{ parent }
    {
    }

    inline auto ProxyAllocator::do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result
    {
        return _parent.allocate(request);
    }

    inline void ProxyAllocator::do_deallocate(ice::alloc_result result) noexcept
    {
        _parent.deallocate(result);
    }

} // namespace ice
