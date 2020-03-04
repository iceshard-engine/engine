#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_archetype_index.hxx>

namespace iceshard::ecs
{

    template<typename Fn, typename... ComponentView>
    void for_each_internal(iceshard::ecs::ArchetypeIndex* index, Fn&& fn) noexcept
    {
        core::allocator& alloc = core::memory::globals::default_allocator();

        core::pod::Array<core::stringid_type> component_ids{ alloc };
        core::pod::Array<uint32_t> archetype_block_count{ alloc };
        core::pod::Array<uint32_t> component_offsets{ alloc };
        core::pod::Array<iceshard::ComponentBlock*> blocks{ alloc };

        constexpr static uint32_t component_count = sizeof...(ComponentView);
        constexpr static uint32_t component_sizes[] = {
            static_cast<uint32_t>(sizeof(ComponentView))...
        };

        core::memory::stack_allocator_1024 temp_alloc;
        core::pod::Array<void*> pointers{ temp_alloc };
        core::pod::array::resize(pointers, component_count);

        constexpr static core::stringid_type component_ids_array[] = {
            std::remove_pointer_t<ComponentView>::identifier...
        };

        for (core::stringid_arg_type cid : component_ids_array)
        {
            core::pod::array::push_back(component_ids, cid);
        }

        index->query_instances(component_ids, archetype_block_count, component_offsets, blocks);

        uint32_t const archetype_count = core::pod::array::size(archetype_block_count);
        uint32_t block_base_idx = 0;

        for (uint32_t arch_idx = 0; arch_idx < archetype_count; ++arch_idx)
        {
            uint32_t const block_count = archetype_block_count[arch_idx];

            for (uint32_t block_idx = 0; block_idx < block_count; ++block_idx)
            {
                iceshard::ComponentBlock* const block = blocks[block_base_idx + block_idx];

                for (uint32_t component_idx = 0; component_idx < component_count; ++component_idx)
                {
                    pointers[component_idx] = core::memory::utils::pointer_add(block, component_offsets[component_idx]);
                }

                uint32_t call_index = 0;
                std::forward<Fn>(fn)(block->_entity_count, ((ComponentView*)nullptr, (ComponentView)pointers[call_index])...);

                //{ block->_entity_count }
                //;
                //for (uint32_t inst_idx = 0; inst_idx < block->_entity_count; ++inst_idx)
                //{
                //}
            }

            block_base_idx += block_count;
        }
    }

} // namespace iceshard::ecs
