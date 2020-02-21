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

        [[nodiscard]]
        auto alloc_block(
            uint32_t const* alignments,
            uint32_t const* sizes,
            uint32_t array_count,
            void** pointers
        ) noexcept -> ComponentBlock*;

        template<typename... Args>
        [[nodiscard]]
        auto alloc_arrays(Args&... pointers) noexcept -> ComponentBlock*;

        void release_block(ComponentBlock* block) noexcept;

    private:
        core::allocator& _allocator;
        std::atomic<ComponentBlock*> _block_list = nullptr;
    };

    template<typename... Args>
    auto ComponentBlockAllocator::alloc_arrays(Args&... pointers) noexcept -> ComponentBlock*
    {
        uint32_t const alignments[] = { alignof(std::remove_pointer_t<Args>) ... };
        uint32_t const sizes[] = { sizeof(std::remove_pointer_t<Args>) ... };
        void** pointers_array[] = { reinterpret_cast<void**>(&pointers) ... };

        return alloc_block(alignments, sizes, sizeof...(Args), *pointers_array);
    }

} // namespace iceshard
