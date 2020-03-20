#include <iceshard/component/component_block_operation.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/pod/algorithm.hxx>
#include <numeric>

namespace iceshard
{

    namespace detail
    {

        auto align_forward_offset(uint32_t offset, uint32_t align) noexcept -> uint32_t
        {
            const uint32_t mod = offset % align;
            if (mod)
                offset += (align - mod);
            return offset;
        }

        void component_block_prepare(
            ComponentBlock* block,
            uint32_t const* sizes,
            uint32_t const* alignments,
            uint32_t array_count,
            uint32_t* offsets
        ) noexcept
        {
            uint32_t const size_sum = std::accumulate(sizes, sizes + array_count, 0u);
            uint32_t const alignment_sum = std::accumulate(alignments, alignments + array_count, 0u);
            uint32_t const available_memory = (block->_block_size - sizeof(ComponentBlock)) - alignment_sum;

            uint32_t base_offset = core::memory::utils::pointer_distance(block, block + 1);

            // Calculate number of components able to store in this block.
            block->_entity_count_max = available_memory / size_sum;

            for (uint32_t component_idx = 0; component_idx < array_count; ++component_idx)
            {
                offsets[component_idx] = align_forward_offset(base_offset, alignments[component_idx]);
                base_offset += sizes[component_idx] * block->_entity_count_max;
            }
        }

        void component_block_prepare(
            ComponentBlock* block,
            uint32_t const* sizes,
            uint32_t const* alignments,
            uint32_t array_count,
            void** pointers
        ) noexcept
        {
            uint32_t const size_sum = std::accumulate(sizes, sizes + array_count, 0u);
            uint32_t const alignment_sum = std::accumulate(alignments, alignments + array_count, 0u);
            uint32_t const available_memory = (block->_block_size - sizeof(ComponentBlock)) - alignment_sum;

            void* memory_pointer = block + 1;

            // Calculate number of components able to store in this block.
            block->_entity_count_max = available_memory / size_sum;

            for (uint32_t component_idx = 0; component_idx < array_count; ++component_idx)
            {
                pointers[component_idx] = core::memory::utils::align_forward(memory_pointer, alignments[component_idx]);
                memory_pointer = core::memory::utils::pointer_add(memory_pointer, sizes[component_idx] * block->_entity_count_max);
            }
        }

    } // namespace detail

    auto component_block_insert(ComponentBlock* block, uint32_t entity_count, uint32_t& first_index) noexcept -> uint32_t
    {
        uint32_t const available_entity_count = block->_entity_count_max - block->_entity_count;

        first_index = block->_entity_count;
        if (available_entity_count >= entity_count) // [[likely]]
        {
            block->_entity_count += entity_count;
            return entity_count;
        }
        else
        {
            block->_entity_count = block->_entity_count_max;
            return available_entity_count;
        }
    }

} // namespace iceshard
