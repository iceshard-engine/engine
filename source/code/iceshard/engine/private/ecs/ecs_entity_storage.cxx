/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/assert.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/profiler.hxx>

namespace ice::ecs
{

    using ice::ecs::detail::ArchetypeInstance;
    using ice::ecs::detail::ArchetypeInstanceInfo;

    namespace detail
    {

        struct OperationDetails
        {
            ice::u32 block_offset;
            void* block_data;
        };

        auto get_entity_array(
            ice::ecs::EntityOperations::ComponentInfo const& info,
            ice::ecs::detail::OperationDetails const& data_details,
            ice::u32 entity_count
        ) noexcept -> ice::Span<ice::ecs::Entity>
        {
            void* const entity_array_ptr = ice::ptr_add(
                data_details.block_data,
                { info.offsets[0] }
            );

            return ice::Span<ice::ecs::Entity>(
                reinterpret_cast<ice::ecs::Entity*>(entity_array_ptr) + data_details.block_offset,
                entity_count
            );
        }

        void track_entities(
            ice::HashMap<ice::ecs::Entity>& tracked,
            ice::Span<ice::ecs::Entity const> updated
        ) noexcept;

        void store_entities_with_data(
            ice::Span<ice::ecs::Entity const> src_entities,
            ice::Span<ice::ecs::EntityDataSlot> dst_data_slots,
            ice::ecs::EntityDataSlot base_slot,
            ice::ecs::EntityOperations::ComponentInfo const& src_info,
            ice::ecs::EntityOperations::ComponentInfo const& dst_info,
            ice::ecs::detail::OperationDetails const& src_data_details,
            ice::ecs::detail::OperationDetails const& dst_data_details
        ) noexcept
        {
            IPT_ZONE_SCOPED;
            ice::u32 const entity_count = ice::count(src_entities);

            ice::u32 const src_component_count = ice::count(src_info.names);
            ice::u32 const dst_component_count = ice::count(dst_info.names);
            ice::u32 const max_component_count = ice::max(src_component_count, dst_component_count);
            bool const source_is_smaller = src_component_count < max_component_count;

            ice::u32 src_component_index = 0;
            ice::u32 dst_component_index = 0;

            ice::u32& main_component_index = (source_is_smaller ? dst_component_index : src_component_index);
            ice::u32& sub_component_index = (source_is_smaller ? src_component_index : dst_component_index);

            // Iterate over each component in the source archetype
            for (; main_component_index < max_component_count && src_component_index < src_component_count; ++main_component_index)
            {
                ice::u32 const dst_size = dst_info.sizes[dst_component_index];
                ice::u32 const dst_offset = dst_info.offsets[dst_component_index] + dst_size * dst_data_details.block_offset;

                // If this is a tag component skip it.
                if (dst_offset == ice::u32_max)
                {
                    ICE_ASSERT(
                        dst_size == 0,
                        "A critical error occured, this offset should only be used for 'tag components' which have size and alignment set to '0'"
                    );
                    continue;
                }

                // If components do not match, skip the copy and clear memory
                if (src_info.names[src_component_index] != dst_info.names[dst_component_index])
                {
                    void* const ptr = ice::ptr_add(dst_data_details.block_data, { dst_offset });

                    ice::memset(ptr, '\0', dst_size * entity_count);
                    continue;
                }

                ice::u32 const src_size = src_info.sizes[src_component_index];
                ice::u32 const src_offset = src_info.offsets[src_component_index] + src_size * src_data_details.block_offset;

                ICE_ASSERT(
                    src_size == dst_size,
                    "Mismatched data size {} != {} for components with the same ID. source: {} ({}), destination: {} ({})",
                    src_size, dst_size,
                    ice::stringid_hint(src_info.names[src_component_index]),
                    ice::hash(src_info.names[src_component_index]),
                    ice::stringid_hint(dst_info.names[dst_component_index]),
                    ice::hash(dst_info.names[dst_component_index])
                );

                void const* const src_ptr = ice::ptr_add(src_data_details.block_data, { src_offset });
                void* const dst_ptr = ice::ptr_add(dst_data_details.block_data, { dst_offset });

                // Do a plain copy (we require components to be standard layout and trivially copyable)
                ice::memcpy(dst_ptr, src_ptr, src_size * entity_count);

                // Increment the sub index
                sub_component_index += 1;
            }

            ice::Span<ice::ecs::Entity> const dst_entities = ice::ecs::detail::get_entity_array(
                dst_info,
                dst_data_details,
                entity_count
            );

            for (ice::u32 idx = 0; idx < entity_count; ++idx)
            {
                base_slot.index = dst_data_details.block_offset + idx;

                ICE_ASSERT(
                    base_slot.archetype < (1 << ice::ecs::Constant_EntityDataSlotArchetype_Bits),
                    "Invalid archetype instance value!"
                );
                ICE_ASSERT(
                    base_slot.block < (1 << ice::ecs::Constant_EntityDataSlotBlock_Bits),
                    "Invalid archetype instance value!"
                );
                ICE_ASSERT(
                    base_slot.index < (1 << ice::ecs::Constant_EntityDataSlotIndex_Bits),
                    "Invalid archetype instance value!"
                );

                // Update data slots
                dst_entities[idx] = src_entities[idx];

                ice::ecs::EntityInfo const entity_info = ice::ecs::entity_info(src_entities[idx]);
                dst_data_slots[entity_info.index] = base_slot;
            }
        }

        void update_entities_with_data(
            ice::u32 entity_count,
            ice::ecs::EntityOperations::ComponentInfo const& src_info,
            ice::ecs::EntityOperations::ComponentInfo const& dst_info,
            ice::ecs::detail::OperationDetails const& src_data_details,
            ice::ecs::detail::OperationDetails const& dst_data_details
        ) noexcept
        {
            IPT_ZONE_SCOPED;
            ice::u32 const src_component_count = ice::count(src_info.names);
            ice::u32 const dst_component_count = ice::count(dst_info.names);
            ice::u32 const max_component_count = ice::max(src_component_count, dst_component_count);
            bool const source_is_smaller = src_component_count < max_component_count;

            ice::u32 src_component_index = 0;
            ice::u32 dst_component_index = 0;

            ice::u32& main_component_index = (source_is_smaller ? dst_component_index : src_component_index);
            ice::u32& sub_component_index = (source_is_smaller ? src_component_index : dst_component_index);

            // Iterate over each component in the source archetype
            for (; main_component_index < max_component_count && src_component_index < src_component_count; ++main_component_index)
            {
                ice::u32 const dst_size = dst_info.sizes[dst_component_index];
                ice::u32 const dst_offset = dst_info.offsets[dst_component_index] + dst_size * dst_data_details.block_offset;

                // If this is a tag component skip it.
                if (dst_offset == ice::u32_max)
                {
                    ICE_ASSERT(
                        dst_size == 0,
                        "A critical error occured, this offset should only be used for 'tag components' which have size and alignment set to '0'"
                    );
                    continue;
                }

                // If components do not match, skip any operation, we only update for data we have
                if (src_info.names[src_component_index] != dst_info.names[dst_component_index])
                {
                    continue;
                }

                ice::u32 const src_size = src_info.sizes[src_component_index];
                ice::u32 const src_offset = src_info.offsets[src_component_index] + src_size * src_data_details.block_offset;

                ICE_ASSERT(
                    src_size == dst_size,
                    "Mismatched data size {} != {} for components with the same ID. source: {} ({}), destination: {} ({})",
                    src_size, dst_size,
                    ice::stringid_hint(src_info.names[src_component_index]),
                    ice::hash(src_info.names[src_component_index]),
                    ice::stringid_hint(dst_info.names[dst_component_index]),
                    ice::hash(dst_info.names[dst_component_index])
                );

                void const* const src_ptr = ice::ptr_add(src_data_details.block_data, { src_offset });
                void* const dst_ptr = ice::ptr_add(dst_data_details.block_data, { dst_offset });

                // Do a plain copy (we require components to be standard layout and trivially copyable)
                ice::memcpy(dst_ptr, src_ptr, src_size * entity_count);

                // Increment the sub index
                sub_component_index += 1;
            }
        }

        void store_entities_without_data(
            ice::Span<ice::ecs::Entity const> src_entities,
            ice::Span<ice::ecs::EntityDataSlot> dst_data_slots,
            ice::ecs::EntityDataSlot base_slot,
            ice::ecs::EntityOperations::ComponentInfo const& info,
            ice::ecs::detail::OperationDetails const& data_details
        ) noexcept
        {
            IPT_ZONE_SCOPED;
            ice::u32 const entity_count = ice::count(src_entities);
            ice::u32 const component_count = ice::count(info.names);

            // Iterate over each component in the source archetype
            for (ice::u32 component_index = 0; component_index < component_count; ++component_index)
            {
                ice::u32 const size = info.sizes[component_index];
                ice::u32 const offset = info.offsets[component_index] + size * data_details.block_offset;

                // If this is a tag component skip it.
                if (offset == ice::u32_max)
                {
                    ICE_ASSERT(
                        size == 0,
                        "A critical error occured, this offset should only be used for 'tag components' which have size and alignment set to '0'"
                    );
                    continue;
                }

                void* const ptr = ice::ptr_add(data_details.block_data, { offset });

                ice::memset(ptr, '\0', size * entity_count);
            }

            ice::Span<ice::ecs::Entity> const dst_entities = ice::ecs::detail::get_entity_array(
                info,
                data_details,
                entity_count
            );

            for (ice::u32 idx = 0; idx < entity_count; ++idx)
            {
                base_slot.index = data_details.block_offset + idx;

                ICE_ASSERT(
                    base_slot.archetype < (1 << ice::ecs::Constant_EntityDataSlotArchetype_Bits),
                    "Invalid archetype instance value!"
                );
                ICE_ASSERT(
                    base_slot.block < (1 << ice::ecs::Constant_EntityDataSlotBlock_Bits),
                    "Invalid archetype instance value!"
                );
                ICE_ASSERT(
                    base_slot.index < (1 << ice::ecs::Constant_EntityDataSlotIndex_Bits),
                    "Invalid archetype instance value!"
                );

                // Update data slots
                dst_entities[idx] = src_entities[idx];

                ice::ecs::EntityInfo const entity_info = ice::ecs::entity_info(src_entities[idx]);
                dst_data_slots[entity_info.index] = base_slot;
            }
        }

        void batch_remove_entities(
            ice::ecs::ArchetypeIndex const& archetypes,
            ice::ecs::EntityOperation const& operation,
            ice::Span<ice::ecs::Entity const> entities_to_remove,
            ice::Span<ice::ecs::DataBlock*> data_blocks,
            ice::Span<ice::ecs::EntityDataSlot> data_slots,
            ice::ShardContainer& out_shards
        ) noexcept
        {
            auto const* it = ice::span::begin(entities_to_remove);
            auto const* const end = ice::span::end(entities_to_remove);

            ArchetypeInstance archetype{};
            ArchetypeInstanceInfo const* archetype_infos[1]{ nullptr };

            ice::ecs::DataBlock* archetype_block = nullptr;
            ice::u32 archetype_block_index = ice::u32_max;

            do
            {
                EntityInfo entity_info = ice::ecs::entity_info(*it);
                EntityDataSlot const slot_info = data_slots[entity_info.index];

                // Find the archetype info if it changed (Users are encouraged to avoid this)
                ArchetypeInstance const temp_archetype[1]{ ArchetypeInstance{ slot_info.archetype } };
                if (archetype != temp_archetype[0])
                {
                    archetypes.fetch_archetype_instance_infos(temp_archetype, archetype_infos);
                    archetype = temp_archetype[0];
                }

                if (slot_info.block != archetype_block_index)
                {
                    // Save the block index
                    ice::u32 const archetype_instance_idx = static_cast<ice::u32>(archetype_infos[0]->archetype_instance);
                    archetype_block_index = slot_info.block;
                    archetype_block = data_blocks[archetype_instance_idx];

                    // Get the proper block
                    ice::u32 block_idx = slot_info.block;
                    while(block_idx > 0)
                    {
                        block_idx -= 1; // Reduce the block index

                        // Get the next block
                        archetype_block = archetype_block->next;
                    }
                }

                ICE_ASSERT_CORE(archetype_block != nullptr);
                ICE_ASSERT(
                    archetype_block->block_entity_count > slot_info.index,
                    "This storage has no data associated with the given entity handle!"
                );

                ice::ecs::detail::OperationDetails del_data_details{
                    .block_offset = slot_info.index, // Initial index
                    .block_data = archetype_block->block_data, // The actual data
                };

                // Batch removal and step to next
                ice::u32 span_size = 1;
                while(it != end)
                {
                    it += 1; // Get the next entity and check if we can already handle it

                    EntityInfo next_entity_info = ice::ecs::entity_info(*it);
                    EntityDataSlot const next_slot_info = data_slots[next_entity_info.index];

                    // Increase count if we found the next entity
                    if ((slot_info.index + span_size) == next_slot_info.index)
                    {
                        span_size += 1;
                    }
                    // Lower offset if we found an entity that is lower later.
                    else if ((del_data_details.block_offset - 1) == next_slot_info.index)
                    {
                        del_data_details.block_offset -= 1;
                    }
                    // Finally break to the outer loop
                    else
                    {
                        break;
                    }
                }

                // Clear the block from the selected entities
                ice::u32 const next_valid_block_idx = del_data_details.block_offset + span_size;
                if (next_valid_block_idx != archetype_block->block_entity_count)
                {
                    // Find how many entities we can move into the newly created hole.
                    ice::ecs::detail::OperationDetails dst_data_details{
                        .block_offset = archetype_block->block_entity_count - 1, // Initial index (last entity)
                        .block_data = archetype_block->block_data, // The actual data
                    };

                    // This allows us to bulk move entities into created holes
                    ice::u32 movable_entities = 1;
                    while(movable_entities < span_size)
                    {
                        // If we did not reach below the valid index, get the next entity to be moved
                        if (dst_data_details.block_offset > next_valid_block_idx)
                        {
                            dst_data_details.block_offset -= 1;
                            movable_entities += 1;
                        }
                    }
                    EntityOperations::ComponentInfo const component_info{
                        .names = archetype_infos[0]->component_identifiers,
                        .sizes = archetype_infos[0]->component_sizes,
                        .offsets = archetype_infos[0]->component_offsets,
                    };

                    // Get moved entity array
                    ice::Span<ice::ecs::Entity const> moved_entities = ice::ecs::detail::get_entity_array(
                        component_info, dst_data_details, movable_entities
                    );

                    // Execute the move
                    ice::ecs::detail::store_entities_with_data(
                        // List of entities that will be moved
                        moved_entities,
                        data_slots,
                        slot_info, // The initially removed slot
                        component_info,
                        component_info,
                        del_data_details, /* src data block */
                        dst_data_details /* dst data block */
                    );
                }

                // Remove entity count from the block we moved from (we can just forget the data existed)
                archetype_block->block_entity_count -= span_size;

            } while(it != end);
        }

    } // namespace detail

    static constexpr ice::ucount Constant_InitialEntityCount = 1024 * 32;

    EntityStorage::EntityStorage(
        ice::Allocator& alloc,
        ice::ecs::ArchetypeIndex const& archetype_index
    ) noexcept
        : _allocator{ alloc, "ecs :: entity-storage" }
        , _entity_index{ _allocator, Constant_InitialEntityCount }
        , _archetype_index{ archetype_index }
        , _access_trackers{ _allocator }
        , _head_blocks{ _allocator }
        , _data_blocks{ _allocator }
        , _data_slots{ _allocator }
    {
        ice::array::reserve(_head_blocks, 100); // 100 archetypes should suffice for now
        ice::array::resize(_data_slots, Constant_InitialEntityCount);
    }

    EntityStorage::~EntityStorage() noexcept
    {
        for (ice::ecs::QueryAccessTracker* tracker : ice::hashmap::values(_access_trackers))
        {
            _allocator.destroy(tracker);
        }

        ice::u32 block_idx = 0;
        for (DataBlock* const data_block : _data_blocks)
        {
            if (data_block->next != nullptr)
            {
                DataBlockPool* block_pool;
                _archetype_index.fetch_archetype_instance_pool(ArchetypeInstance{ block_idx }, block_pool);

                ICE_ASSERT(block_pool != nullptr, "Error while trying to release data block!");

                DataBlock* block_it = data_block->next;
                while (block_it != nullptr)
                {
                    DataBlock* next_block = block_it->next;
                    block_it->next = nullptr;

                    block_pool->release_block(block_it);
                    block_it = next_block;
                }
            }

            block_idx += 1;
        }
    }

    auto EntityStorage::entities() noexcept -> ice::ecs::EntityIndex&
    {
        return _entity_index;
    }

    auto EntityStorage::entities() const noexcept -> ice::ecs::EntityIndex const&
    {
        return _entity_index;
    }

    void EntityStorage::update_archetypes() noexcept
    {
        ice::u32 const existing_count = ice::count(_head_blocks);
        ice::u32 const archetype_count = _archetype_index.registered_archetype_count();
        if (existing_count >= archetype_count)
        {
            return; // Don't remove archetype headblocks because queries might still reference them.
        }

        ice::array::resize(_head_blocks, archetype_count);
        ice::array::resize(_data_blocks, archetype_count);

        // Setup the empty head blocks for new archetypes.
        //  This approach gives two benefits:
        //  1. Queries are returning valid pointers for head blocks, that can be skipped if no entity was ever allocate
        //  2. We dont have any allocation overhead for archetypes that exist bur where never used, or are used rarely.
        for (ice::u32 idx = existing_count; idx < archetype_count; ++idx)
        {
            DataBlock* head_block = ice::addressof(_head_blocks[idx]);
            head_block->block_data = nullptr;
            head_block->block_data_size = 0_B;
            head_block->block_entity_count = 0;
            head_block->block_entity_count_max = 0;
            head_block->next = nullptr;

            _data_blocks[idx] = head_block;

            if (idx == 0)
            {
                continue;
            }

            // Go over each archetype component and create non-existent trackers
            ice::ecs::detail::ArchetypeInstanceInfo const* info = nullptr;
            _archetype_index.fetch_archetype_instance_info_by_index(idx, info);
            ICE_ASSERT_CORE(info != nullptr);

            for (ice::StringID component_id : info->component_identifiers)
            {
                ice::ecs::QueryAccessTracker* tracker = ice::hashmap::get(_access_trackers, ice::hash(component_id), nullptr);
                if (tracker == nullptr)
                {
                    tracker = _allocator.create<ice::ecs::QueryAccessTracker>();
                    ice::hashmap::set(_access_trackers, ice::hash(component_id), tracker);
                }
            }
        }
    }

    void EntityStorage::execute_operations(
        ice::ecs::EntityOperations const& operations,
        ice::ShardContainer& out_shards
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        // [Done] Set Archetype: {EntityHandle[*], DstArchetype, ComponentData[*]} // add
        // [Done] Rep Archetype: {EntityHandle[1], DstArchetype, <implicit: SrcArchetype>, ComponentData[*]} // change
        // [Done] Set Component: {EntityHandle[1], None, ComponentData[*]} // update data
        // [Done] Set Component: {EntityHandle[*], None} // remove

        // Ensure we have enough data slots
        if (ice::array::count(_data_slots) < _entity_index.count())
        {
            ice::array::resize(_data_slots, _entity_index.count());
        }

        // Ensure all queries are finished
        for (ice::ecs::QueryAccessTracker* tracker : ice::hashmap::values(_access_trackers))
        {
            ice::u32 const final_counter_exec = tracker->access_stage_executed.exchange(0, std::memory_order_relaxed);
            ice::u32 const final_counter_next = tracker->access_stage_next.exchange(0, std::memory_order_relaxed);
            ICE_ASSERT_CORE(final_counter_exec == final_counter_next);
        }

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
                    EntityInfo const entity = ice::ecs::entity_info(operation.entities[idx]);
                    EntityDataSlot const slot_info = _data_slots[entity.index];

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
            EntityInfo const handle = ice::ecs::entity_info(operation.entities[0]);
            EntityDataSlot const slot_info = _data_slots[handle.index];

            ArchetypeInstanceInfo const* src_instance_info[1]{ nullptr };
            if (slot_info.archetype != 0)
            {
                ArchetypeInstance const src_isntance[1]{ ArchetypeInstance{ slot_info.archetype } };
                _archetype_index.fetch_archetype_instance_infos(src_isntance, src_instance_info);
            }

            DataBlockPool* dst_instance_pool = nullptr;
            ArchetypeInstanceInfo const* dst_instance_info = nullptr;

            _archetype_index.fetch_archetype_instance_info_with_pool(operation.archetype, dst_instance_info, dst_instance_pool);


            EntityOperations::ComponentInfo const* const provided_component_info = reinterpret_cast<EntityOperations::ComponentInfo const*>(
                operation.component_data
            );

            detail::OperationDetails provided_data_details{
                //.block_size = 0,
                .block_offset = 0,
                .block_data = nullptr
            };

            if (provided_component_info != nullptr)
            {
                provided_data_details.block_data = ice::ptr_add(
                    operation.component_data,
                    ice::size_of<EntityOperations::ComponentInfo>
                );
            }

            // We have a valid destination archetype...
            if (dst_instance_info != nullptr)
            {
                ice::u32 const dst_instance_idx = static_cast<ice::u32>(dst_instance_info->archetype_instance);

                EntityOperations::ComponentInfo const dst_component_info{
                    .names = dst_instance_info->component_identifiers,
                    .sizes = dst_instance_info->component_sizes,
                    .offsets = dst_instance_info->component_offsets,
                };
                detail::OperationDetails dst_data_details{
                    .block_offset = 0,
                    .block_data = nullptr,
                };

                // We need to track the current block index
                ice::u32 block_idx = 0;

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
                ice::u32 processed_count = 0;
                ice::u32 remaining_count = operation.entity_count;

                // Run an operation as long as we have entities to process
                do
                {
                    ice::u32 const available_space = data_block_it->block_entity_count_max - data_block_it->block_entity_count;
                    if (available_space > 0)
                    {
                        dst_data_details.block_data = data_block_it->block_data;
                        dst_data_details.block_offset = data_block_it->block_entity_count;

                        ice::ecs::EntityDataSlot const base_slot{
                            .archetype = dst_instance_idx,
                            .block = block_idx,
                            .index = data_block_it->block_entity_count,
                        };

                        ice::u32 const entities_stored = ice::min(available_space, remaining_count);
                        ice::Span<ice::ecs::Entity const> const entities{
                            operation.entities + processed_count,
                            entities_stored
                        };

                        if (src_instance_info[0] == nullptr)
                        {
                            if (provided_component_info != nullptr)
                            {
                                ice::ecs::detail::store_entities_with_data(
                                    entities,
                                    _data_slots,
                                    base_slot,
                                    *provided_component_info,
                                    dst_component_info,
                                    provided_data_details, /* src data block */
                                    dst_data_details /* dst data block */
                                );
                            }
                            else
                            {
                                ice::ecs::detail::store_entities_without_data(
                                    entities,
                                    _data_slots,
                                    base_slot,
                                    dst_component_info,
                                    dst_data_details /* dst data block */
                                );
                            }
                        }
                        else
                        {
                            // #todo get rid of this simplification at some point? But is it really requied?
                            ICE_ASSERT(
                                operation.entity_count == 1,
                                "It's not allowed to move more than a single entity between archetypes."
                            );

                            ice::u32 const src_instance_idx = static_cast<ice::u32>(src_instance_info[0]->archetype_instance);

                            EntityOperations::ComponentInfo const src_component_info{
                                .names = src_instance_info[0]->component_identifiers,
                                .sizes = src_instance_info[0]->component_sizes,
                                .offsets = src_instance_info[0]->component_offsets,
                            };

                            DataBlock* data_block_it_2 = _data_blocks[src_instance_idx];
                            ICE_ASSERT(
                                data_block_it_2 != nullptr,
                                "This storage has no data associated with the given source archetype!"
                            );

                            uint32_t block_idx_2 = slot_info.block;
                            while (block_idx_2 > 0)
                            {
                                data_block_it_2 = data_block_it_2->next;
                                block_idx_2 -= 1;

                                ICE_ASSERT(
                                    data_block_it_2 != nullptr,
                                    "This storage has no data associated with the given entity handle!"
                                );
                            }

                            ICE_ASSERT(
                                data_block_it_2->block_entity_count > slot_info.index,
                                "This storage has no data associated with the given entity handle!"
                            );

                            detail::OperationDetails src_data_details{
                                .block_offset = slot_info.index,
                                .block_data = data_block_it_2->block_data,
                            };

                            // Moving is generally a very expensive operation as it needs to change both the source archetype storage and the target storage.
                            // 1. First we need to copy all similar data to the target storage.

                            ice::ecs::detail::store_entities_with_data(
                                entities,
                                _data_slots,
                                base_slot,
                                src_component_info,
                                dst_component_info,
                                src_data_details, /* src data block */
                                dst_data_details /* dst data block */
                            );

                            {
                                ice::Span<ice::ecs::Entity const> updated_entities = ice::ecs::detail::get_entity_array(
                                    dst_component_info,
                                    dst_data_details,
                                    entities_stored
                                );
                                ICE_ASSERT(ice::count(updated_entities) == 0, "");
                            }

                            // 2. Apply the new provided data if any.
                            if (provided_component_info != nullptr)
                            {
                                ice::ecs::detail::update_entities_with_data(
                                    operation.entity_count,
                                    *provided_component_info,
                                    dst_component_info,
                                    provided_data_details, /* src data block */
                                    dst_data_details /* dst data block */
                                );
                            }

                            // 3. We need to remove all the copied data from the source storage.
                            //  We only do this if we have more than one entity and if the enitity is not at the end of the block
                            if (data_block_it_2->block_entity_count > 2 && (data_block_it_2->block_entity_count - 1) != src_data_details.block_offset)
                            {
                                dst_data_details = src_data_details;
                                src_data_details.block_offset = data_block_it_2->block_entity_count - 1; // Get the last entity

                                ice::ecs::Entity const move_entities[1]{
                                    ice::span::front(ice::ecs::detail::get_entity_array(src_component_info, src_data_details, 1))
                                };

                                EntityDataSlot const move_slot{
                                    .archetype = src_instance_idx,
                                    .block = slot_info.block,
                                    .index = slot_info.index
                                };

                                ice::ecs::detail::store_entities_with_data(
                                    move_entities,
                                    _data_slots,
                                    move_slot,
                                    src_component_info,
                                    src_component_info,
                                    src_data_details, /* src data block */
                                    dst_data_details /* dst data block */
                                );
                            }

                            // Remove entity count from the block we moved from (we can just forget the data existed)
                            data_block_it_2->block_entity_count -= 1;
                        }

                        // Update the remianing count
                        remaining_count -= entities_stored;
                        data_block_it->block_entity_count += entities_stored;
                    }

                    // Get the next block
                    if (remaining_count > 0)
                    {
                        if (data_block_it->next == nullptr)
                        {
                            data_block_it->next = dst_instance_pool->request_block();

                            data_block_it = data_block_it->next;
                            data_block_it->block_entity_count_max = dst_instance_info->component_entity_count_max;
                            data_block_it->block_entity_count = 0;
                        }
                        else
                        {
                            data_block_it = data_block_it->next;
                        }

                        block_idx += 1;
                    }

                } while (remaining_count > 0);
            }
            else if (src_instance_info[0] != nullptr)
            {
                ice::u32 const src_instance_idx = static_cast<ice::u32>(src_instance_info[0]->archetype_instance);

                EntityOperations::ComponentInfo const src_component_info{
                    .names = src_instance_info[0]->component_identifiers,
                    .sizes = src_instance_info[0]->component_sizes,
                    .offsets = src_instance_info[0]->component_offsets,
                };

                DataBlock* data_block_it = _data_blocks[src_instance_idx];
                ICE_ASSERT(
                    data_block_it != nullptr,
                    "This storage has no data associated with the given source archetype!"
                );

                ice::u32 block_idx = slot_info.block;
                while (block_idx > 0)
                {
                    data_block_it = data_block_it->next;
                    block_idx -= 1;

                    ICE_ASSERT(
                        data_block_it != nullptr,
                        "This storage has no data associated with the given entity handle!"
                    );
                }

                ICE_ASSERT(
                    data_block_it->block_entity_count > slot_info.index,
                    "This storage has no data associated with the given entity handle!"
                );

                detail::OperationDetails src_data_details{
                    .block_offset = slot_info.index,
                    .block_data = data_block_it->block_data,
                };

                // Entered ONLY when want to update the components not the archetype
                if (provided_component_info != nullptr)
                {
                    // #todo get rid of this simplification at some point? But is it really requied?
                    ICE_ASSERT(
                        operation.entity_count == 1,
                        "It's not allowed to update or remove more than a single in a operation."
                    );

                    ice::ecs::detail::update_entities_with_data(
                        operation.entity_count,
                        *provided_component_info,
                        src_component_info,
                        provided_data_details, /* src data block */
                        src_data_details /* dst data block */
                    );
                }
                else
                {

                    // detail::OperationDetails const dst_data_details = src_data_details;

                    ice::Span<ice::ecs::Entity const> entities = ice::ecs::detail::get_entity_array(
                        src_component_info,
                        detail::OperationDetails{ .block_data = operation.entities },
                        operation.entity_count
                    );

                    ice::ecs::detail::batch_remove_entities(
                        _archetype_index,
                        operation,
                        entities,
                        _data_blocks,
                        _data_slots,
                        out_shards
                    );
                }
            }
            else
            {
                ICE_LOG(
                    ice::LogSeverity::Warning, ice::LogTag::Engine,
                    "Trying to execute invalid operation for {} entities. Please check if the operation was properly defined.",
                    operation.entity_count
                );

            }
        }
    }

    auto EntityStorage::find_archetype(
        ice::String name
    ) const noexcept -> ice::ecs::Archetype
    {
        return _archetype_index.find_archetype_by_name(name);
    }

    auto EntityStorage::query_data_slot(
        ice::ecs::Entity entity
    ) const noexcept -> ice::ecs::EntityDataSlot
    {
        if (_entity_index.is_alive(entity))
        {
            ice::ecs::EntityInfo const entity_info = ice::ecs::entity_info(entity);
            return _data_slots[entity_info.index];
        }
        return {};
    }

    auto EntityStorage::query_data_slots(
        ice::Span<ice::ecs::Entity const> requested,
        ice::Span<ice::ecs::EntityDataSlot> out_data_slots
    ) const noexcept -> ice::ucount
    {
        ice::u32 idx = 0;
        ice::ucount valid = 0;
        for (ice::ecs::Entity entity : requested)
        {
            ice::ecs::EntityInfo const entity_info = ice::ecs::entity_info(entity);
            out_data_slots[idx] = _data_slots[entity_info.index];
            valid += _entity_index.is_alive(entity);
            idx += 1;
        }
        return valid;
    }

    void EntityStorage::query_internal(
        ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
        ice::Span<ice::StringID const> query_tags,
        ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
        ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
    ) const noexcept
    {
        IPT_ZONE_SCOPED;
        ice::StackAllocator<512_B> archetypes_alloc{};
        ice::Array<ice::ecs::Archetype> archetypes{ archetypes_alloc };
        ice::array::reserve(archetypes, ice::mem_max_capacity<ice::ecs::Archetype>(archetypes_alloc.Constant_InternalCapacity));
        _archetype_index.find_archetypes(archetypes, query_info, query_tags);

        ice::u32 const prev_archetype_count = ice::count(out_instance_infos);
        ice::u32 const new_archetype_count = ice::count(archetypes);
        ice::array::resize(out_instance_infos, prev_archetype_count + new_archetype_count);
        ice::array::reserve(out_data_blocks, prev_archetype_count + new_archetype_count);

        _archetype_index.fetch_archetype_instance_infos(archetypes, ice::array::slice(out_instance_infos, prev_archetype_count));

        for (ice::ecs::ArchetypeInstanceInfo const* instance : ice::array::slice(out_instance_infos, prev_archetype_count))
        {
            ice::u32 const instance_idx = static_cast<ice::u32>(instance->archetype_instance);
            ice::array::push_back(out_data_blocks, _data_blocks[instance_idx]);
        }

        // Find or create work trackers for queried components
        ice::u32 idx = prev_archetype_count;
        for (ice::ecs::detail::QueryTypeInfo const& type_info : query_info)
        {
            ice::ecs::QueryAccessTracker* tracker = ice::hashmap::get(_access_trackers, ice::hash(type_info.identifier), nullptr);
            ICE_ASSERT_CORE(tracker != nullptr);

            out_access_trackers[idx] = tracker;
            idx += 1;
        }
    }

} // namespace ice::ecs
