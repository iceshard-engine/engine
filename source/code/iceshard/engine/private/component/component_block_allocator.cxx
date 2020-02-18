#include <iceshard/component/component_block_allocator.hxx>
#include <iceshard/component/component_block.hxx>

namespace iceshard
{

    ComponentBlockAllocator::ComponentBlockAllocator(core::allocator& alloc) noexcept
        : _allocator{ alloc }
    {
    }

    ComponentBlockAllocator::~ComponentBlockAllocator() noexcept
    {
        auto* block = _block_list.load();
        while (block != nullptr)
        {
            auto* block_to_release = block;
            block = block->_next;

            _allocator.destroy(block_to_release);
        }
    }

    auto ComponentBlockAllocator::alloc_block() noexcept -> ComponentBlock*
    {
        ComponentBlock* free_block = _block_list.load(std::memory_order_relaxed);
        if (free_block != nullptr)
        {
            if (false == _block_list.compare_exchange_strong(free_block, free_block->_next, std::memory_order_relaxed))
            {
                free_block = nullptr;
            }
        }

        if (free_block == nullptr)
        {
            free_block = _allocator.make<ComponentBlock>();
        }

        free_block->_next = nullptr;
        return free_block;
    }

    void ComponentBlockAllocator::release_block(ComponentBlock* block) noexcept
    {
        IS_ASSERT(block->_next == nullptr, "Only last tail blocks can be released!");

        block->_next = _block_list.load(std::memory_order_relaxed);
        while (false == _block_list.compare_exchange_weak(block->_next, block, memory_order_relaxed))
        {
            block->_next = _block_list.load(std::memory_order_relaxed);
        }
    }

} // namespace iceshard
