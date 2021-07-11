#pragma once
#include <ice/entity/entity_storage.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/pod/array.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

namespace ice
{

    namespace detail
    {

        struct EntityInstanceInfo
        {
            ice::ArchetypeHandle archetype;
            ice::u16 block;
            ice::u16 index;
        };

        auto get_instance_info(EntityInstance instance) noexcept -> EntityInstanceInfo
        {
            union
            {
                EntityInstance instance;
                EntityInstanceInfo info;
            } helper{ instance };
            return helper.info;
        }

        auto make_instance(EntityInstanceInfo instance_info) noexcept -> EntityInstance
        {
            union
            {
                EntityInstanceInfo info;
                EntityInstance instance;
            } helper{ instance_info };
            return helper.instance;
        }

        auto get_entity_location(
            ice::ArchetypeInfo const& archetype,
            ice::ArchetypeBlock* block,
            ice::u32 index
        ) noexcept -> ice::Entity*
        {
            void* result_location = ice::memory::ptr_add(
                block->block_data,
                archetype.offsets[0] + archetype.sizes[0] * index
            );
            return reinterpret_cast<ice::Entity*>(result_location);
        }

        void prepare_entity_data(
            ice::Entity entity,
            ice::ArchetypeInfo const& archetype,
            ice::ArchetypeBlock* block,
            ice::u32 index
        ) noexcept
        {
            ice::Entity* entity_location = get_entity_location(archetype, block, index);
            *entity_location = entity;

            // Iterate over each component in the source archetype
            ice::u32 const component_count = ice::size(archetype.components);
            for (ice::u32 component_idx = 1; component_idx < component_count; ++component_idx)
            {
                ice::u32 const size = archetype.sizes[component_idx];
                ice::u32 const offset = archetype.offsets[component_idx] + size * index;

                void* const data_location = ice::memory::ptr_add(block->block_data, offset);
                ice::memset(data_location, '\0', size);
            }
        }

        bool find_free_block(ice::ArchetypeBlock*& block, ice::u32& block_index) noexcept
        {
            block_index = 0;
            while (block != nullptr && block->entity_count == block->entity_count_max)
            {
                block_index += 1;
                block = block->next;
            }

            return block != nullptr;
        }

        void find_or_allocate_block(
            ice::ArchetypeInfo const& archetype,
            ice::ArchetypeBlock*& out_block,
            ice::u32& out_block_index
        ) noexcept
        {
            ice::ArchetypeBlock* first_block = out_block;
            if (detail::find_free_block(out_block, out_block_index) == false)
            {
                out_block_index = 0;
                if (first_block != nullptr)
                {
                    out_block = first_block;
                    while (out_block->next != nullptr)
                    {
                        out_block = out_block->next;
                        out_block_index += 1;
                    }

                    out_block_index += 1;
                }

                ice::ArchetypeBlock* new_block = archetype.block_allocator->allocate_block();
                new_block->entity_count_max = archetype.block_max_entity_count;
                new_block->block_data = ice::memory::ptr_align_forward(
                    new_block->block_data, archetype.block_base_alignment
                );

                if (out_block != nullptr)
                {
                    out_block->next = new_block;
                }
                out_block = new_block;
            }
        }

        bool get_block_at_index(ice::ArchetypeBlock*& block, ice::u32 block_index) noexcept
        {
            while (block != nullptr && block_index > 0)
            {
                block = block->next;
                block_index -= 1;
            }
            return block_index == 0;
        }

    } // namespace detail

    EntityStorage::EntityStorage(
        ice::Allocator& alloc,
        ice::ArchetypeIndex& archetype_index,
        ice::ArchetypeBlockAllocator& archetype_block_alloc
    ) noexcept
        : _allocator{ alloc }
        , _archetype_index{ archetype_index }
        //, _archetype_block_alloc{ archetype_block_alloc }
        , _archetype_blocks{ alloc }
        , _instances{ alloc }
    {
    }

    EntityStorage::~EntityStorage() noexcept
    {
        for (auto const& entry : _archetype_blocks)
        {
            ice::ArchetypeHandle const handle[1]{ static_cast<ice::ArchetypeHandle>(entry.key) };
            ice::ArchetypeInfo info[1];
            _archetype_index.archetype_info(handle, info);

            ice::ArchetypeBlock* block = entry.value;
            while (block != nullptr)
            {
                ice::ArchetypeBlock* next_block = block->next;
                block->next = nullptr; // TODO: Remove the tail block removal assert?
                info[0].block_allocator->release_block(block);
                block = next_block;
            }
        }
    }

    auto EntityStorage::archetype_index() noexcept -> ice::ArchetypeIndex&
    {
        return _archetype_index;
    }

    void EntityStorage::set_archetype(
        ice::Entity entity,
        ice::ArchetypeHandle archetype
    ) noexcept
    {
        EntityInstance const instance = ice::pod::hash::get(
            _instances,
            ice::hash(entity),
            EntityInstance::Invalid
        );

        detail::EntityInstanceInfo info = detail::get_instance_info(instance);
        if (info.archetype == archetype)
        {
            return;
        }

        ice::ArchetypeHandle const archetypes[2]{
            archetype,
            info.archetype
        };
        ice::ArchetypeInfo archetype_info[2]{ };
        _archetype_index.archetype_info(archetypes, archetype_info);

        if (info.archetype != ArchetypeHandle::Invalid)
        {
            ice::ArchetypeBlock* old_block = ice::pod::hash::get(
                _archetype_blocks,
                ice::hash(info.archetype),
                nullptr
            );

            if (detail::get_block_at_index(old_block, info.block))
            {
                ice::u32 const source_index = old_block->entity_count - 1;

                if (source_index != info.index)
                {
                    detail::EntityDataOperation move_operation{ };
                    move_operation.source_archetype = &archetype_info[1];
                    move_operation.source_block = old_block;
                    move_operation.source_index = source_index;
                    move_operation.destination_archetype = &archetype_info[1];
                    move_operation.destination_block = old_block;
                    move_operation.destination_index = info.index;

                    detail::move_entity_data(move_operation);
                }

                old_block->entity_count -= 1;
            }
            else
            {
                ICE_LOG(
                    ice::LogSeverity::Error, ice::LogTag::Engine,
                    "Encountered invalid Archetype entity instance. This entity seems to be dead?"
                );
            }
        }

        // Get the destination archetype block
        ice::ArchetypeBlock* dst_block = ice::pod::hash::get(
            _archetype_blocks,
            ice::hash(archetype),
            nullptr
        );

        ice::u32 block_index;
        detail::find_or_allocate_block(
            archetype_info[0],
            dst_block,
            block_index
        );

        if (block_index == 0)
        {
            ice::pod::hash::set(
                _archetype_blocks,
                ice::hash(archetype),
                dst_block
            );
        }

        // Update the instance informations
        info.archetype = archetype;
        info.block = block_index;
        info.index = dst_block->entity_count;

        detail::prepare_entity_data(
            entity,
            archetype_info[0],
            dst_block,
            info.index
        );

        // Increment the entity count
        dst_block->entity_count += 1;

        ice::pod::hash::set(
            _instances,
            ice::hash(entity),
            detail::make_instance(info)
        );
    }

    void EntityStorage::set_archetypes(
        ice::ArchetypeHandle archetype,
        ice::Span<ice::Entity const> entities,
        ice::ArchetypeOperation operation
    ) noexcept
    {
        // TODO: Remove that const cast for the operation.
        ice::detail::ArchetypeConstBlock const data_operation_block{
            .entity_count = operation.source_entity_count,
            .block_size = operation.source_data.size,
            .block_data = operation.source_data.location,
        };
        ice::detail::ArchetypeDataOperation data_operation{
            .source_archetype = operation.source_archetype,
            .source_block = &data_operation_block,
            .source_offset = operation.source_data_offset,
            .source_count = operation.source_entity_count,
        };
        //ice::pod::Array<ice::detail::ArchetypeDataOperation> data_operations{ _allocator };
        //ice::pod::array::push_back(data_operations,
        //    ice::detail::ArchetypeDataOperation
        //    {
        //        .source_archetype = operation.source_archetype,
        //        .source_block = operation.source_block,
        //        .source_offset = 0,
        //        .source_count = 0,
        //    }
        //);

        for (ice::Entity entity : entities)
        {
            ICE_ASSERT(
                ice::pod::hash::has(_instances, ice::hash(entity)) == false,
                "Cannot set an archetype in bulk for entities that already have an archetype!"
            );
        }

        ice::ArchetypeInfo archetype_infos[1]{ };
        ice::ArchetypeHandle const archetype_handles[1]{ archetype };
        if (_archetype_index.archetype_info(archetype_handles, archetype_infos) == false)
        {
            // TODO: assert?
            return;
        }

        // Get the destination archetype block
        ice::ArchetypeBlock* dst_block = ice::pod::hash::get(
            _archetype_blocks,
            ice::hash(archetype),
            nullptr
        );

        ice::u32 block_index;
        detail::find_or_allocate_block(
            archetype_infos[0],
            dst_block,
            block_index
        );

        if (block_index == 0)
        {
            ice::pod::hash::set(
                _archetype_blocks,
                ice::hash(archetype),
                dst_block
            );
        }

        ice::ArchetypeBlock* const first_block = ice::pod::hash::get(
            _archetype_blocks,
            ice::hash(archetype),
            nullptr
        );

        ice::u32 const predicted_instance_count = ice::u32(
            ice::f32(ice::pod::array::size(_instances._hash) + data_operation.source_count) / 0.6f
        );
        ice::pod::hash::reserve(_instances, predicted_instance_count);

        do
        {
            dst_block = first_block;
            detail::find_or_allocate_block(
                archetype_infos[0],
                dst_block,
                block_index
            );

            data_operation.destination_archetype = archetype_infos + 0;
            data_operation.destination_block = dst_block;
            data_operation.destination_offset = dst_block->entity_count;
            data_operation.destination_count = ice::min(dst_block->entity_count_max - dst_block->entity_count, data_operation.source_count);
            dst_block->entity_count += data_operation.destination_count;

            detail::EntityInstanceInfo instance_info;
            instance_info.archetype = archetype;
            instance_info.block = block_index;
            instance_info.index = data_operation.destination_offset;

            // Create a instance for each entity we just wrote to
            for (ice::u32 entity_idx = 0; entity_idx < data_operation.destination_count; ++entity_idx)
            {
                ice::pod::hash::set(
                    _instances,
                    ice::hash(entities[entity_idx]),
                    detail::make_instance(instance_info)
                );

                instance_info.index += 1;
            }

            data_operation = detail::copy_archetype_data(data_operation);

        } while (data_operation.source_count > 0);
    }

    void EntityStorage::change_archetype(
        ice::Entity entity,
        ice::ArchetypeHandle new_archetype
    ) noexcept
    {
        EntityInstance const instance = ice::pod::hash::get(
            _instances,
            ice::hash(entity),
            EntityInstance::Invalid
        );

        detail::EntityInstanceInfo info = detail::get_instance_info(instance);
        if (info.archetype == new_archetype)
        {
            return;
        }

        ice::ArchetypeHandle const archetypes[2]{
            info.archetype,
            new_archetype
        };
        ice::ArchetypeInfo archetype_info[2]{ };
        _archetype_index.archetype_info(archetypes, archetype_info);

        // Get the destination archetype block
        ice::ArchetypeBlock* dst_block = ice::pod::hash::get(
            _archetype_blocks,
            ice::hash(new_archetype),
            nullptr
        );

        ice::u32 dst_block_index;
        detail::find_or_allocate_block(
            archetype_info[1],
            dst_block,
            dst_block_index
        );
        ice::u32 const dst_index = dst_block->entity_count;

        // Copy data for all common components
        if (info.archetype != ArchetypeHandle::Invalid)
        {
            ice::ArchetypeBlock* src_block = ice::pod::hash::get(
                _archetype_blocks,
                ice::hash(info.archetype),
                nullptr
            );

            if (detail::get_block_at_index(src_block, info.block))
            {
                // Copy to new archetype block
                detail::EntityDataOperation move_operation{ };
                move_operation.source_archetype = &archetype_info[0];
                move_operation.source_block = src_block;
                move_operation.source_index = info.index;
                move_operation.destination_archetype = &archetype_info[1];
                move_operation.destination_block = dst_block;
                move_operation.destination_index = dst_index;
                detail::copy_entity_data(move_operation);

                // Move entities in old block
                ice::u32 const source_index = src_block->entity_count - 1;
                if (source_index != info.index)
                {
                    move_operation.source_archetype = &archetype_info[0];
                    move_operation.source_block = src_block;
                    move_operation.source_index = src_block->entity_count - 1;
                    move_operation.destination_archetype = &archetype_info[0];
                    move_operation.destination_block = src_block;
                    move_operation.destination_index = info.index;
                    detail::move_entity_data(move_operation);
                }

                src_block->entity_count -= 1;
            }
            else
            {
                ICE_LOG(
                    ice::LogSeverity::Error, ice::LogTag::Engine,
                    "Encountered invalid Archetype entity instance. This entity seems to be dead?"
                );
            }
        }

        if (dst_block_index == 0)
        {
            ice::pod::hash::set(
                _archetype_blocks,
                ice::hash(new_archetype),
                dst_block
            );
        }

        // Update the instance informations
        info.archetype = new_archetype;
        info.block = dst_block_index;
        info.index = dst_index;

        // Increment the entity count
        dst_block->entity_count += 1;

        ice::pod::hash::set(
            _instances,
            ice::hash(entity),
            detail::make_instance(info)
        );
    }

    void EntityStorage::set_components(
        ice::Entity entity,
        ice::Span<ice::ArchetypeComponent const> components
    ) noexcept
    {
        ice::u32 const component_count = ice::size(components);

        ice::pod::Array<ice::StringID> names{ _allocator };
        ice::pod::Array<bool> optionality{ _allocator };

        ice::pod::array::resize(names, component_count);
        ice::pod::array::resize(optionality, component_count);

        for (ice::u32 idx = 0; idx < component_count; ++idx)
        {
            names[idx] = ice::StringID{ components[idx].name };
            optionality[idx] = false;
        }

        ice::ArchetypeQueryCriteria const query{
            .components = ice::Span<ice::StringID>{ ice::pod::array::begin(names), component_count },
            .optional = ice::Span<bool>{ ice::pod::array::begin(optionality), component_count },
        };

        ice::ArchetypeHandle archetype = _archetype_index.find_archetype(query);
        if (archetype != ArchetypeHandle::Invalid)
        {
            change_archetype(entity, archetype);
        }
    }

    void EntityStorage::add_component(
        ice::Entity entity,
        ice::ArchetypeComponent component
    ) noexcept
    {
        EntityInstance const instance = ice::pod::hash::get(
            _instances,
            ice::hash(entity),
            EntityInstance::Invalid
        );

        ice::pod::Array<ice::StringID> new_components{ _allocator };

        detail::EntityInstanceInfo info = detail::get_instance_info(instance);
        if (info.archetype == ArchetypeHandle::Invalid)
        {
            ice::pod::array::reserve(new_components, 2);
            ice::pod::array::push_back(new_components,
                ice::StringID{ ice::Archetype<>().components[0].name }
            );
            ice::pod::array::push_back(new_components, ice::StringID{ component.name });
            // This array is already sorted so we dont need to do it.
        }
        else
        {
            ice::ArchetypeHandle archetype_handles[1]{ info.archetype };
            ice::ArchetypeInfo archetype_info[1]{ };
            _archetype_index.archetype_info(archetype_handles, archetype_info);

            ice::pod::array::reserve(new_components, ice::size(archetype_info[0].components) + 1);
            ice::pod::array::push_back(new_components, archetype_info[0].components);
            ice::pod::array::push_back(new_components, ice::StringID{ component.name });
            ice::detail::sort_component_array(new_components);
        }

        ice::ArchetypeHandle const new_archetype = _archetype_index.find_archetype(
            ice::ArchetypeQueryCriteria{
                .components = ice::Span<ice::StringID>{ ice::pod::array::begin(new_components), ice::pod::array::size(new_components) }
            }
        );

        if (info.archetype == ArchetypeHandle::Invalid)
        {
            set_archetype(entity, new_archetype);
        }
        else
        {
            change_archetype(entity, new_archetype);
        }
    }

    void EntityStorage::remove_component(
        ice::Entity entity,
        ice::StringID_Arg component_name
    ) noexcept
    {
        // TODO Assert cannot remove entity component

        EntityInstance const instance = ice::pod::hash::get(
            _instances,
            ice::hash(entity),
            EntityInstance::Invalid
        );

        ice::pod::Array<ice::StringID> new_components{ _allocator };

        detail::EntityInstanceInfo info = detail::get_instance_info(instance);
        if (info.archetype == ArchetypeHandle::Invalid)
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "Tryint to remove component {} ({}) from entity without an archetype!",
                ice::stringid_hint(component_name), ice::hash(component_name)
            );
            return;
        }

        ice::ArchetypeHandle archetype_handle[1]{ info.archetype };
        ice::ArchetypeInfo archetype_info[1]{ };
        _archetype_index.archetype_info(archetype_handle, archetype_info);

        ice::pod::array::reserve(new_components, ice::size(archetype_info[0].components));
        for (ice::StringID_Arg component : archetype_info[0].components)
        {
            if (component != component_name)
            {
                ice::pod::array::push_back(new_components, component);
            }
        }

        bool const removed_anything = ice::size(archetype_info[0].components) > ice::pod::array::size(new_components);
        if (removed_anything == false)
        {
            return;
        }

        if (ice::pod::array::size(new_components) <= 1)
        {
            // TODO assert if first component is entity
            erase_data(entity);
        }
        else
        {
            ice::detail::sort_component_array(new_components);
            ice::ArchetypeHandle const new_archetype = _archetype_index.find_archetype(
                ice::ArchetypeQueryCriteria{
                    .components = ice::Span<ice::StringID>{ ice::pod::array::begin(new_components), ice::pod::array::size(new_components) }
                }
            );

            change_archetype(entity, new_archetype);
        }
    }

    void EntityStorage::erase_data(ice::Entity entity) noexcept
    {
        EntityInstance const instance = ice::pod::hash::get(
            _instances,
            ice::hash(entity),
            EntityInstance::Invalid
        );

        detail::EntityInstanceInfo info = detail::get_instance_info(instance);
        if (info.archetype == ArchetypeHandle::Invalid)
        {
            return;
        }

        ice::ArchetypeHandle archetype_handle[1]{ info.archetype };
        ice::ArchetypeInfo archetype_info[1]{ };
        _archetype_index.archetype_info(archetype_handle, archetype_info);

        ice::ArchetypeBlock* old_block = ice::pod::hash::get(
            _archetype_blocks,
            ice::hash(info.archetype),
            nullptr
        );

        if (detail::get_block_at_index(old_block, info.block))
        {
            ice::u32 const source_index = old_block->entity_count - 1;

            if (source_index != info.index)
            {
                detail::EntityDataOperation move_operation{ };
                move_operation.source_archetype = &archetype_info[0];
                move_operation.source_block = old_block;
                move_operation.source_index = source_index;
                move_operation.destination_archetype = &archetype_info[0];
                move_operation.destination_block = old_block;
                move_operation.destination_index = info.index;

                detail::move_entity_data(move_operation);
            }

            old_block->entity_count -= 1;
        }
        else
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "Encountered invalid Archetype entity instance. This entity seems to be dead?"
            );
        }
    }

    void EntityStorage::query_blocks(
        ice::Span<ice::ArchetypeHandle const> archetypes,
        ice::pod::Array<ice::u32>& archetype_block_count,
        ice::pod::Array<ice::ArchetypeBlock*>& archetype_blocks
    ) noexcept
    {
        for (ice::ArchetypeHandle const handle : archetypes)
        {
            ice::u32 block_count = 0;
            ice::ArchetypeBlock* block = ice::pod::hash::get(
                _archetype_blocks,
                ice::hash(handle),
                nullptr
            );

            while (block != nullptr)
            {
                if (block->entity_count > 0)
                {
                    block_count += 1;
                    ice::pod::array::push_back(
                        archetype_blocks,
                        block
                    );
                }
                block = block->next;
            }

            ice::pod::array::push_back(
                archetype_block_count,
                block_count
            );
        }
    }

    //bool EntityStorage::query_entity(
    //    ice::Entity entity,
    //    ice::ArchetypeInfo* archetype_info_out,
    //    ice::ArchetypeBlock* entity_block_out,
    //    ice::u32 entity_index_out
    //) noexcept
    //{
    //    ice::EntityInstance instance = ice::pod::hash::get(_instances, ice::hash(entity), ice::EntityInstance::Invalid);

    //    ice::detail::EntityInstanceInfo instance_info = detail::get_instance_info(instance);
    //    if (instance_info.archetype == ArchetypeHandle::Invalid)
    //    {
    //        return false;
    //    }

    //    ice::ArchetypeBlock* block = ice::pod::hash::get(_archetype_blocks, ice::hash(instance_info.archetype), nullptr);
    //    if (detail::get_block_at_index(block, instance_info.block) == false)
    //    {
    //        ICE_LOG(
    //            ice::LogSeverity::Error, ice::LogTag::Engine,
    //            "Encountered invalid Archetype entity instance. This entity [{:x}] seems to be dead?",
    //            ice::hash(entity)
    //        );
    //        return false;
    //    }

    //    ice::pod::Array<ice::ArchetypeInfo> archetype_info{ _allocator };
    //    bool const found_archetype = _archetype_index.archetype_info(
    //        ice::Span<ArchetypeHandle const>{ &instance_info.archetype, 1 },
    //        archetype_info
    //    );

    //    if (found_archetype == false)
    //    {
    //        return false;
    //    }

    //    *archetype_info_out = archetype_info[0];
    //    *entity_block_out = *block; // makes a copy
    //    entity_block_out->next = nullptr;
    //    entity_index_out = instance_info.index;
    //    return true;
    //}

} // namespace ice
