#pragma once
#include <core/allocator.hxx>
#include <atomic>
#include <typeindex>
#include <fmt/format.h>

namespace iceshard
{

    struct ComponentBlock;

    class ComponentBlockAllocator final
    {
    public:
        ComponentBlockAllocator(core::allocator& alloc) noexcept;
        ~ComponentBlockAllocator() noexcept;

        auto alloc_block() noexcept -> ComponentBlock*;

        void release_block(ComponentBlock* block) noexcept;

    private:
        core::allocator& _allocator;
        std::atomic<ComponentBlock*> _block_list = nullptr;
    };

} // namespace iceshard
