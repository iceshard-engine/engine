#include <ice/archetype/archetype_block_allocator.hxx>
#include <ice/assert.hxx>

namespace ice
{

    ArchetypeBlockAllocator::ArchetypeBlockAllocator(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
    {
    }

    ArchetypeBlockAllocator::~ArchetypeBlockAllocator() noexcept
    {
        ice::ArchetypeBlock* block = _block_list.load();
        while (block != nullptr)
        {
            auto* block_to_release = block;
            block = block->next;

            _allocator.deallocate(block_to_release);
        }
    }

    auto ArchetypeBlockAllocator::block_size() const -> ice::u32
    {
        return 16 * 1024 - sizeof(ArchetypeBlock);
    }

    auto ArchetypeBlockAllocator::allocate_block() noexcept -> ice::ArchetypeBlock*
    {
        ArchetypeBlock* free_block = _block_list.load(std::memory_order_relaxed);
        if (free_block != nullptr)
        {
            if (false == _block_list.compare_exchange_strong(free_block, free_block->next, std::memory_order_relaxed))
            {
                free_block = nullptr;
            }
        }

        if (free_block == nullptr)
        {
            void* block_data = _allocator.allocate(16 * 1024); // 16 KB blocks
            free_block = reinterpret_cast<ArchetypeBlock*>(block_data);
            free_block->block_size = 16 * 1024 - sizeof(ArchetypeBlock);
            free_block->block_data = free_block + 1;
        }

        free_block->entity_count_max = 0;
        free_block->entity_count = 0;
        free_block->next = nullptr;
        return free_block;
    }

    void ArchetypeBlockAllocator::release_block(ice::ArchetypeBlock * block) noexcept
    {
        ICE_ASSERT(block->next == nullptr, "Only tail blocks can be released!");

        block->next = _block_list.load(std::memory_order_relaxed);
        while (false == _block_list.compare_exchange_weak(block->next, block, std::memory_order_relaxed))
        {
            block->next = _block_list.load(std::memory_order_relaxed);
        }
    }

} // namespace ice
