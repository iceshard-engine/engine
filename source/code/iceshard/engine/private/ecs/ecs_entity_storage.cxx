#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    namespace detail
    {



    } // namespace detail

    EntityStorage::EntityStorage(
        ice::Allocator& alloc,
        ice::ecs::ArchetypeIndex const& archetype_index
    ) noexcept
        : _allocator{ alloc }
        , _archetype_index{ archetype_index }
        , _data_blocks{ _allocator }
        //, _archetype_heads{ _allocator }
    {
        ice::pod::array::resize(_data_blocks, _archetype_index.registered_archetype_count());

        // Set the whole array to nullptrs.
        for (DataBlock*& data_block_ptr : _data_blocks)
        {
            data_block_ptr = nullptr;
        }
        //ice::pod::hash::reserve(_archetype_heads, ice::pod::array::capacity(_data_blocks) / 0.6f);
    }

    void EntityStorage::execute_operations(
        ice::ecs::EntityOperations const& operations
    ) noexcept
    {
        // Set Archetype: {EntityHandle[], DstArchetype, ComponentData[]} // add
        // Rep Archetype: {EntityHandle[], DstArchetype, <implicit: SrcArchetype>, ComponentData[]} // change
        // Set Component: {EntityHandle[], None, ComponentData[]} // update data
        // Set Component: {EntityHandle[], None} // remove

        for (EntityOperation const& operation : operations)
        {
            if (operation.entity_count == 0)
            {
                ICE_LOG(
                    ice::LogSeverity::Error, ice::LogTag::Engine,
                    "Ill-formed entity operation, no entities found! Skipping..."
                );
                continue;
            }

            ICE_LOG(
                ice::LogSeverity::Debug, ice::LogTag::Engine,
                "Executing operation with {} entities.",
                operation.entity_count
            );

            if constexpr (ice::build::is_debug || ice::build::is_develop)
            {
                bool same_archetype = true;
                ice::u32 expected_archetype_index = 0;

                // #todo: Performance warning check.
                //  This might have quite a big performance impact if done from multiple source archetypes.
                for (ice::u32 idx = 0; idx < operation.entity_count && same_archetype; ++idx)
                {
                    EntityHandle const handle = operation.entities[idx];
                    EntitySlotInfo const slot_info = ice::ecs::entity_slot_info(ice::ecs::entity_handle_info(handle).slot);

                    if (expected_archetype_index == 0)
                    {
                        expected_archetype_index = slot_info.archetype;
                    }
                    else
                    {
                        same_archetype = (expected_archetype_index == slot_info.archetype);
                    }
                }

                // #todo allow different archetypes maybe?
                ICE_ASSERT(
                    same_archetype == true,
                    "Entities in operation have different archetypes, operation is illformed!"
                );
            }

            // Get the archetype from the first entity, we are ensured such an entity exists.
            EntityHandle const handle = operation.entities[0];
            EntityHandleInfo const handle_info = ice::ecs::entity_handle_info(handle);
            EntitySlotInfo const slot_info = ice::ecs::entity_slot_info(handle_info.slot);

            ArchetypeInstance const src_isntance[1]{ ArchetypeInstance{ slot_info.archetype } };
            ArchetypeInstanceInfo const* src_instance_info[1]{ nullptr };
            if (slot_info.archetype != 0)
            {
                _archetype_index.fetch_archetype_instance_infos(src_isntance, src_instance_info);
            }

            DataBlockPool* dst_instance_pool = nullptr;
            ArchetypeInstanceInfo* dst_instance_info = nullptr;

            // #todo: this function assumes that the archetype is valid, otherwise it asserts. Maybe we should allow invalid archetypes?
            _archetype_index.fetch_archetype_instance_info_with_pool(operation.archetype, dst_instance_info, dst_instance_pool);

            // We have a valid destination archetype...
            if (dst_instance_info != nullptr)
            {
                ice::u32 const dst_instance_idx = static_cast<ice::u32>(dst_instance_info->archetype_instance);

                // We need at least the initial data block.
                DataBlock* data_block_it = _data_blocks[dst_instance_idx];
                if (data_block_it == nullptr)
                {
                    data_block_it = dst_instance_pool->request_block();
                    data_block_it->block_entity_count_max = dst_instance_info->component_entity_count_max;
                    data_block_it->block_entity_count = 0;

                    _data_blocks[dst_instance_idx] = data_block_it;
                }

                // Number of remaining entities to process
                ice::u32 remaining_count = operation.entity_count;

                // Run an operation as long as we have entities to process
                do
                {
                    ice::u32 const available_space = data_block_it->block_entity_count_max - data_block_it->block_entity_count;
                    if (available_space > 0)
                    {
                        ice::u32 const entities_stored = ice::min(available_space, remaining_count);

                        if (src_instance_info[0] != nullptr)
                        {
                            // todo: move entities
                        }
                        else
                        {
                            // todo: create entities
                        }

                        // Update the remianing count
                        remaining_count -= entities_stored;
                        data_block_it->block_entity_count += entities_stored;
                    }

                    if (remaining_count > 0 && data_block_it->next == nullptr)
                    {
                        data_block_it->next = dst_instance_pool->request_block();

                        data_block_it = data_block_it->next;
                        data_block_it->block_entity_count_max = dst_instance_info->component_entity_count_max;
                        data_block_it->block_entity_count = 0;
                    }

                } while (false);
            }
            else
            {
                // Remove or Update entities
            }

        }
    }

} // namespace ice::ecs
