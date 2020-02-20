#include <iceshard/component/component_block_allocator.hxx>
#include <iceshard/component/component_block.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/pod/algorithm.hxx>
#include <numeric>

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

            _allocator.deallocate(block_to_release);
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
            void* block_data = _allocator.allocate(16 * 1024); // 16 KB blocks
            free_block = reinterpret_cast<ComponentBlock*>(block_data);
            free_block->_block_size = 16 * 1024;
        }

        free_block->_entity_count_max = 0;
        free_block->_entity_count = 0;
        free_block->_next = nullptr;
        return free_block;
    }

    auto ComponentBlockAllocator::alloc_block(
        uint32_t const* alignments,
        uint32_t const* sizes,
        uint32_t array_count,
        void** pointers
    ) noexcept -> ComponentBlock*
    {
        ComponentBlock* free_block = alloc_block();

        uint32_t const size_sum = std::accumulate(sizes, sizes + array_count, 0u);
        uint32_t const alignment_sum = std::accumulate(alignments, alignments + array_count, 0u);
        uint32_t const available_memory = (free_block->_block_size - sizeof(ComponentBlock)) - alignment_sum;

        void* memory_pointer = free_block + 1;

        // Calculate number of components able to store in this block.
        free_block->_entity_count_max = available_memory / size_sum;

        // Update pointers properly
        struct PointerData
        {
            uint32_t size;
            uint32_t alignment;
            void** ptr;
        };

        core::memory::stack_allocator<256> pointer_alloc;
        core::pod::Array<PointerData> pointers_data{ pointer_alloc };
        core::pod::array::resize(pointers_data, array_count);

        for (uint32_t idx = 0; idx < array_count; ++idx)
        {
            pointers_data[idx].size = sizes[idx];
            pointers_data[idx].alignment = alignments[idx];
            pointers_data[idx].ptr = &pointers[idx];
        }

        core::pod::sort(pointers_data, [](PointerData const& left, PointerData const& right) noexcept -> int
            {
                return left.alignment >= right.alignment;
            });

        for (PointerData& pointer_data : pointers_data)
        {
            *pointer_data.ptr = core::memory::utils::align_forward(memory_pointer, pointer_data.alignment);
            memory_pointer = core::memory::utils::pointer_add(*pointer_data.ptr, pointer_data.size * free_block->_entity_count_max);
        }
        return free_block;
    }


    void ComponentBlockAllocator::release_block(ComponentBlock* block) noexcept
    {
        IS_ASSERT(block->_next == nullptr, "Only last tail blocks can be released!");

        block->_next = _block_list.load(std::memory_order_relaxed);
        while (false == _block_list.compare_exchange_weak(block->_next, block, std::memory_order_relaxed))
        {
            block->_next = _block_list.load(std::memory_order_relaxed);
        }
    }

} // namespace iceshard
