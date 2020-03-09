#include "iceshard_archetype_index.hxx"
#include "component_archetype_data.hxx"

#include <iceshard/component/component_block_operation.hxx>
#include <core/memory.hxx>
#include <core/allocators/stack_allocator.hxx>

namespace iceshard::ecs
{

    namespace detail
    {

        void set_instance_entity(
            ArchetypeInstance instance,
            Entity entity
        ) noexcept
        {
            ArchetypeData const* const archetype = instance.archetype;

            auto const offset = archetype->offsets[0] + archetype->sizes[0] * instance.index;
            auto const ptr = core::memory::utils::pointer_add(
                instance.block,
                offset
            );

            *reinterpret_cast<Entity*>(ptr) = entity;
        }

        auto get_instance_entity(
            ArchetypeInstance instance
        ) noexcept -> Entity
        {
            ArchetypeData const* const archetype = instance.archetype;

            auto const offset = archetype->offsets[0] + archetype->sizes[0] * instance.index;
            auto const ptr = core::memory::utils::pointer_add(
                instance.block,
                offset
            );

            return *reinterpret_cast<Entity*>(ptr);
        }

        struct ArchetypeDataOperation
        {
            iceshard::ecs::ArchetypeData* src_archetype;
            iceshard::ComponentBlock* src_block;
            uint32_t src_index;

            iceshard::ecs::ArchetypeData* dst_archetype;
            iceshard::ComponentBlock* dst_block;
            uint32_t dst_index;

            uint32_t count;
        };

        void clear_data(
            ArchetypeDataOperation const& operation
        ) noexcept
        {
            ArchetypeData const* const archetype_data = operation.dst_archetype;
            uint32_t const component_count = core::pod::array::size(archetype_data->components);

            // Iterate over each component in the source archetype
            for (uint32_t component_idx = 0; component_idx < component_count; ++component_idx)
            {
                uint32_t const size = archetype_data->sizes[component_idx];
                uint32_t const dst_offset = archetype_data->offsets[component_idx] + size * operation.dst_index;

                void* const dst_ptr = core::memory::utils::pointer_add(operation.dst_block, dst_offset);

                // Do a plain copy (we require components to be standard layout and trivially copyable)
                memset(dst_ptr, '\0', size);
            }
        }

        void copy_data(
            ArchetypeDataOperation const& operation
        ) noexcept
        {
            if (operation.src_archetype == operation.dst_archetype)
            {
                ArchetypeData const* const archetype_data = operation.src_archetype;
                uint32_t const component_count = core::pod::array::size(archetype_data->components);

                // Iterate over each component in the source archetype
                for (uint32_t component_idx = 0; component_idx < component_count; ++component_idx)
                {
                    uint32_t const size = archetype_data->sizes[component_idx];
                    uint32_t const src_offset = archetype_data->offsets[component_idx] + size * operation.src_index;
                    uint32_t const dst_offset = archetype_data->offsets[component_idx] + size * operation.dst_index;

                    void* const src_ptr = core::memory::utils::pointer_add(operation.src_block, src_offset);
                    void* const dst_ptr = core::memory::utils::pointer_add(operation.dst_block, dst_offset);

                    // Do a plain copy (we require components to be standard layout and trivially copyable)
                    memcpy(dst_ptr, src_ptr, size);
                }
            }
            else
            {
                ArchetypeData const* const src_archetype_data = operation.src_archetype;
                ArchetypeData const* const dst_archetype_data = operation.dst_archetype;

                uint32_t const src_component_count = core::pod::array::size(src_archetype_data->components);
                uint32_t const dst_component_count = core::pod::array::size(dst_archetype_data->components);
                uint32_t const max_component_count = std::max(src_component_count, dst_component_count);

                bool const source_is_smaller = src_component_count < max_component_count;

                uint32_t src_component_index = 0;
                uint32_t dst_component_index = 0;

                uint32_t& main_component_index = (source_is_smaller ? dst_component_index : src_component_index);
                uint32_t& sub_component_index = (source_is_smaller ? src_component_index : dst_component_index);

                // Iterate over each component in the source archetype
                for (; main_component_index < max_component_count; ++main_component_index)
                {
                    // If components do not match, skip the copy
                    if (src_archetype_data->components[src_component_index] != dst_archetype_data->components[dst_component_index])
                    {
                        uint32_t const size = dst_archetype_data->sizes[dst_component_index];
                        uint32_t const offset = dst_archetype_data->offsets[dst_component_index] + size * operation.dst_index;
                        void* const ptr = core::memory::utils::pointer_add(operation.dst_block, offset);

                        memset(ptr, '\0', size);
                        continue;
                    }

                    IS_ASSERT(
                        src_archetype_data->sizes[src_component_index] == dst_archetype_data->sizes[dst_component_index],
                        "Mismatched data size {} != {} for components with the same ID. source: {}, destination: {}",
                        src_archetype_data->sizes[src_component_index], dst_archetype_data->sizes[dst_component_index],
                        src_archetype_data->components[src_component_index], dst_archetype_data->components[dst_component_index]
                    );

                    uint32_t const size = src_archetype_data->sizes[src_component_index];
                    uint32_t const src_offset = src_archetype_data->offsets[src_component_index] + size * operation.src_index;
                    uint32_t const dst_offset = dst_archetype_data->offsets[dst_component_index] + size * operation.dst_index;

                    void* const src_ptr = core::memory::utils::pointer_add(operation.src_block, src_offset);
                    void* const dst_ptr = core::memory::utils::pointer_add(operation.dst_block, dst_offset);

                    // Do a plain copy (we require components to be standard layout and trivially copyable)
                    memcpy(dst_ptr, src_ptr, size);

                    // Increment the sub index
                    sub_component_index += 1;
                }
            }
        }

        void create_instances(
            ArchetypeData* archetype_data,
            uint32_t count,
            core::pod::Array<ArchetypeInstance>& base_instances,
            core::pod::Array<uint32_t>& counts
        ) noexcept
        {
            ComponentBlock* block = archetype_data->block;
            while (count > 0)
            {
                if (block->_entity_count == block->_entity_count_max)
                {
                    block->_next = archetype_data->block_allocator->alloc_block();
                    block->_next->_entity_count_max = block->_entity_count_max;
                    block = block->_next;
                    block->_next = nullptr;
                }

                uint32_t const available_count = block->_entity_count_max - block->_entity_count;
                uint32_t const allocated_count = std::min(available_count, count);

                ArchetypeInstance instance;
                instance.archetype = archetype_data;
                instance.block = block;
                instance.index = block->_entity_count;
                block->_entity_count += allocated_count;

                core::pod::array::push_back(base_instances, instance);
                core::pod::array::push_back(counts, allocated_count);

                count -= allocated_count;
            }
        }

        auto create_instance(
            ArchetypeData* archetype_data
        ) noexcept -> ArchetypeInstance
        {
            ComponentBlock* block = archetype_data->block;
            if (block->_entity_count == block->_entity_count_max)
            {
                block->_next = archetype_data->block_allocator->alloc_block();
                block->_next->_entity_count_max = block->_entity_count_max;
                block = block->_next;
                block->_next = nullptr;
            }

            ArchetypeInstance instance;
            instance.archetype = archetype_data;
            instance.block = block;
            instance.index = block->_entity_count;
            block->_entity_count += 1;

            return instance;
        }

        bool remove_instance(
            ArchetypeInstance instance
        ) noexcept
        {
            ComponentBlock* block = instance.block;

            IS_ASSERT(
                block->_entity_count > 0,
                "Cannot remove instance from block with no entities."
            );

            block->_entity_count -= 1;
            if (instance.index < block->_entity_count)
            {
                ArchetypeDataOperation copy_op;
                copy_op.src_archetype = instance.archetype;
                copy_op.src_block = block;
                copy_op.src_index = block->_entity_count;
                copy_op.dst_archetype = instance.archetype;
                copy_op.dst_block = block;
                copy_op.dst_index = instance.index;
                copy_op.count = 1;

                copy_data(copy_op);
                return true;
            }

            return false;
        }

        bool query_component_indices(
            core::pod::Array<core::stringid_type> const& available_components,
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<uint32_t>& indices
        ) noexcept
        {
            uint32_t const available_component_count = core::pod::array::size(available_components);
            uint32_t const required_component_count = core::pod::array::size(required_components);

            if (available_component_count < required_component_count)
            {
                return false;
            }

            IS_ASSERT(
                core::pod::array::empty(indices),
                "The array used to store indices needs to be empty!"
            );
            core::pod::array::reserve(indices, required_component_count);

            for (uint32_t idx = 0; idx < available_component_count; ++idx)
            {
                for (core::stringid_arg_type required_component : required_components)
                {
                    if (available_components[idx] == required_component)
                    {
                        core::pod::array::push_back(indices, idx);
                        break;
                    }
                }
            }

            return core::pod::array::size(indices) == required_component_count;
        }

        template<typename T, uint32_t Size>
        auto single_value_array(T value, core::memory::stack_allocator<Size>& alloc) noexcept -> core::pod::Array<T>
        {
            core::pod::Array<T> result{ core::memory::globals::null_allocator() };
            result._data = reinterpret_cast<T*>(alloc.allocate(sizeof(T), alignof(T)));
            result._capacity = 1;
            result._size = 1;
            result._data[0] = value;
            return result;
        }

    } // namespace detail

    IceShardArchetypeIndex::IceShardArchetypeIndex(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc
    ) noexcept
        : _allocator{ alloc }
        , _block_allocator{ block_alloc }
        , _archetype_data{ _allocator }
        , _entity_archetype{ _allocator }
    {
        core::memory::stack_allocator<128> temp_alloc;
        auto base_component = detail::single_value_array<core::stringid_type>("isc.entity"_sid, temp_alloc);
        auto base_size = detail::single_value_array<uint32_t>(sizeof(iceshard::Entity), temp_alloc);
        auto base_alignment = detail::single_value_array<uint32_t>(alignof(iceshard::Entity), temp_alloc);

        _root_archetype = iceshard::ecs::create_archetype(
            _allocator,
            _block_allocator,
            base_component,
            base_size,
            base_alignment
        );

        core::pod::hash::set(
            _archetype_data,
            core::hash(_root_archetype->identifier),
            _root_archetype
        );
    }

    IceShardArchetypeIndex::~IceShardArchetypeIndex() noexcept
    {
        for (auto const& data : _archetype_data)
        {
            iceshard::ComponentBlock* block = data.value->block;
            while (block != nullptr)
            {
                auto* next = block->_next;
                block->_next = nullptr;
                _block_allocator->release_block(block);
                block = next;
            }

            _allocator.destroy(data.value);
        }
    }

    void IceShardArchetypeIndex::add_entity(
        iceshard::Entity entity,
        core::stringid_arg_type archetype
    ) noexcept
    {
        auto const hash_entity = core::hash(entity);
        auto const hash_archetype = core::hash(archetype);

        IS_ASSERT(
            core::pod::hash::has(_entity_archetype, hash_entity) == false,
            "Entity {} has already an archetype set: {}",
            entity,
            core::stringid_invalid
        );
        IS_ASSERT(
            core::pod::hash::has(_archetype_data, hash_archetype),
            "Archetype {} does not exist in the index",
            archetype
        );

        auto instance = detail::create_instance(
            core::pod::hash::get(_archetype_data, hash_archetype, nullptr)
        );

        {
            detail::ArchetypeDataOperation clear_op;
            clear_op.dst_archetype = instance.archetype;
            clear_op.dst_block = instance.block;
            clear_op.dst_index = instance.index;
            clear_op.count = 1;
            detail::clear_data(clear_op);
        }

        detail::set_instance_entity(
            instance,
            entity
        );

        core::pod::hash::set(
            _entity_archetype,
            hash_entity,
            instance
        );
    }

    void IceShardArchetypeIndex::add_entities(
        core::pod::Array<iceshard::Entity> const& entities,
        core::stringid_arg_type archetype
    ) noexcept
    {
        auto const hash_archetype = core::hash(archetype);

        iceshard::ecs::ArchetypeData* const archetype_data = core::pod::hash::get(
            _archetype_data,
            hash_archetype,
            nullptr
        );

        IS_ASSERT(
            archetype_data != nullptr,
            "Archetype {} does not exist in the index",
            archetype
        );

        uint32_t const requested_entity_count = core::pod::array::size(entities);
        uint32_t const block_count = (requested_entity_count / archetype_data->block->_entity_count_max) + 1;

        core::pod::Array<detail::ArchetypeInstance> instances{ _allocator };
        core::pod::Array<uint32_t> counts{ _allocator };
        core::pod::array::reserve(instances, block_count);
        core::pod::array::reserve(counts, block_count);

        detail::create_instances(archetype_data, requested_entity_count, instances, counts);

        uint32_t const final_block_count = core::pod::array::size(instances);
        iceshard::Entity const* entity_it = core::pod::array::begin(entities);

        for (uint32_t idx = 0; idx < final_block_count; ++idx)
        {
            detail::ArchetypeInstance const& base_instance = instances[idx];

            void* const dst_entity_data = core::memory::utils::pointer_add(
                base_instance.block,
                archetype_data->offsets[0] + archetype_data->sizes[0] * base_instance.index
            );

            uint32_t const copy_count = (base_instance.block->_entity_count - base_instance.index);

            // Copy all identifiers to the first component which is always the `isc.entity` component
            memcpy(dst_entity_data, entity_it, archetype_data->sizes[0] * copy_count);
            entity_it += copy_count;

            // Clear data for all other components
            uint32_t const component_count = core::pod::array::size(archetype_data->components);
            for (uint32_t cmp_idx = 1; cmp_idx < component_count; ++cmp_idx)
            {
                void* const dst_data = core::memory::utils::pointer_add(
                    base_instance.block,
                    archetype_data->offsets[cmp_idx] + archetype_data->sizes[cmp_idx] * base_instance.index
                );

                memset(dst_data, '\0', archetype_data->sizes[cmp_idx] * copy_count);
            }

            // Save entity archetype
            for (uint32_t entity_idx = 0; entity_idx < copy_count; ++entity_idx)
            {
                core::pod::hash::set(
                    _entity_archetype,
                    *(reinterpret_cast<uint64_t*>(dst_entity_data) + entity_idx),
                    base_instance
                );
            }
        }
    }

    void IceShardArchetypeIndex::remove_entity(
        iceshard::Entity entity
    ) noexcept
    {
        auto const hash_entity = core::hash(entity);

        IS_ASSERT(
            core::pod::hash::has(_entity_archetype, hash_entity) == true,
            "Entity {} does not have any archetype set",
            entity
        );

        auto const instance = core::pod::hash::get(_entity_archetype, hash_entity, detail::ArchetypeInstance{ });
        auto const data_moved = detail::remove_instance(instance);
        if (data_moved)
        {
            auto moved_entity = detail::get_instance_entity(instance);
            core::pod::hash::set(
                _entity_archetype,
                core::hash(moved_entity),
                instance
            );
        }

        core::pod::hash::remove(
            _entity_archetype,
            hash_entity
        );
    }

    void IceShardArchetypeIndex::add_component(
        iceshard::Entity entity,
        core::stringid_arg_type component,
        uint32_t size,
        uint32_t alignment
    ) noexcept
    {
        auto const hash_entity = core::hash(entity);

        detail::ArchetypeInstance const src_instance = core::pod::hash::get(
            _entity_archetype,
            hash_entity,
            detail::ArchetypeInstance{ _root_archetype }
        );

        ArchetypeData* const archetype = get_or_create_archetype_extended(src_instance.archetype, component, size, alignment);
        detail::ArchetypeInstance dst_instance = detail::create_instance(archetype);

        detail::ArchetypeDataOperation copy_or_clear_op;
        copy_or_clear_op.src_archetype = src_instance.archetype;
        copy_or_clear_op.src_block = src_instance.block;
        copy_or_clear_op.src_index = src_instance.index;
        copy_or_clear_op.dst_archetype = dst_instance.archetype;
        copy_or_clear_op.dst_block = dst_instance.block;
        copy_or_clear_op.dst_index = dst_instance.index;
        copy_or_clear_op.count = 1;

        if (copy_or_clear_op.src_block != nullptr)
        {
            detail::copy_data(copy_or_clear_op);

            auto const data_moved = detail::remove_instance(src_instance);
            if (data_moved)
            {
                auto moved_entity = detail::get_instance_entity(src_instance);
                core::pod::hash::set(
                    _entity_archetype,
                    core::hash(moved_entity),
                    src_instance
                );
            }
        }
        else
        {
            detail::clear_data(copy_or_clear_op);

            detail::set_instance_entity(
                dst_instance,
                entity
            );
        }

        core::pod::hash::set(
            _entity_archetype,
            hash_entity,
            dst_instance
        );
    }

    void IceShardArchetypeIndex::remove_component(
        iceshard::Entity entity,
        core::stringid_arg_type component
    ) noexcept
    {
        auto const hash_entity = core::hash(entity);

        detail::ArchetypeInstance const src_instance = core::pod::hash::get(
            _entity_archetype,
            hash_entity,
            detail::ArchetypeInstance{ _root_archetype }
        );

        ArchetypeData* const archetype = get_or_create_archetype_reduced(src_instance.archetype, component);
        detail::ArchetypeInstance dst_instance = detail::create_instance(archetype);

        detail::ArchetypeDataOperation copy_op;
        copy_op.src_archetype = src_instance.archetype;
        copy_op.src_block = src_instance.block;
        copy_op.src_index = src_instance.index;
        copy_op.dst_archetype = dst_instance.archetype;
        copy_op.dst_block = dst_instance.block;
        copy_op.dst_index = dst_instance.index;
        copy_op.count = 1;

        detail::copy_data(copy_op);

        auto const data_moved = detail::remove_instance(src_instance);
        if (data_moved)
        {
            auto moved_entity = detail::get_instance_entity(src_instance);
            core::pod::hash::set(
                _entity_archetype,
                core::hash(moved_entity),
                src_instance
            );
        }

        core::pod::hash::set(
            _entity_archetype,
            hash_entity,
            dst_instance
        );
    }

    auto IceShardArchetypeIndex::get_or_create_archetype_extended(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::stringid_arg_type component,
        uint32_t size,
        uint32_t alignment
    ) noexcept -> iceshard::ecs::ArchetypeData*
    {
        core::memory::stack_allocator<64> temp_alloc;
        auto components = detail::single_value_array<core::stringid_type>(component, temp_alloc);
        auto sizes = detail::single_value_array(size, temp_alloc);
        auto alignments = detail::single_value_array(alignment, temp_alloc);

        auto const archetype = iceshard::ecs::archetype_identifier_extended(base_archetype, components);
        auto const hash_archetype = core::hash(archetype);

        ArchetypeData* result = core::pod::hash::get(_archetype_data, hash_archetype, nullptr);
        if (nullptr == result)
        {
            result = iceshard::ecs::create_archetype_extended(
                _allocator,
                _block_allocator,
                base_archetype,
                components,
                sizes,
                alignments
            );

            core::pod::hash::set(
                _archetype_data,
                hash_archetype,
                result
            );
        }
        return result;
    }

    auto IceShardArchetypeIndex::get_or_create_archetype_reduced(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::stringid_arg_type component
    ) noexcept -> iceshard::ecs::ArchetypeData*
    {
        core::memory::stack_allocator<64> temp_alloc;
        auto components = detail::single_value_array<core::stringid_type>(component, temp_alloc);

        auto const archetype = iceshard::ecs::archetype_identifier_reduced(base_archetype, components);
        auto const hash_archetype = core::hash(archetype);

        ArchetypeData* result = core::pod::hash::get(_archetype_data, hash_archetype, nullptr);
        if (nullptr == result)
        {
            result = iceshard::ecs::create_archetype_reduced(
                _allocator,
                _block_allocator,
                base_archetype,
                components
            );

            core::pod::hash::set(
                _archetype_data,
                hash_archetype,
                result
            );
        }
        return result;
    }

    auto IceShardArchetypeIndex::create_archetype(
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t> const& sizes,
        core::pod::Array<uint32_t> const& alignments
    ) noexcept -> core::stringid_type
    {
        uint32_t const component_count = core::pod::array::size(components);
        uint32_t const sizes_count = core::pod::array::size(sizes);
        uint32_t const alignments_count = core::pod::array::size(alignments);

        IS_ASSERT(
            component_count == sizes_count && component_count == alignments_count,
            "Number of sizes ({}) and alignments ({}) do not match with the component count = {}",
            sizes_count,
            alignments_count,
            component_count
        );

        auto const archetype = iceshard::ecs::archetype_identifier_extended(_root_archetype, components);
        auto const hash_archetype = core::hash(archetype);

        ArchetypeData* result = core::pod::hash::get(_archetype_data, hash_archetype, nullptr);
        if (nullptr == result)
        {
            result = iceshard::ecs::create_archetype_extended(
                _allocator,
                _block_allocator,
                _root_archetype,
                components,
                sizes,
                alignments
            );

            core::pod::hash::set(
                _archetype_data,
                hash_archetype,
                result
            );
        }
        return result->identifier;
    }

    auto IceShardArchetypeIndex::get_archetype(
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type
    {
        auto const archetype = iceshard::ecs::archetype_identifier_extended(_root_archetype, components);
        auto const hash_archetype = core::hash(archetype);

        ArchetypeData const* const result = core::pod::hash::get(_archetype_data, hash_archetype, nullptr);
        return result == nullptr ? core::stringid_invalid : result->identifier;
    }

    void IceShardArchetypeIndex::query_instances(
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t>& block_count,
        core::pod::Array<uint32_t>& block_offsets,
        core::pod::Array<iceshard::ComponentBlock*>& blocks
    ) noexcept
    {
        using Entry = core::pod::Hash<iceshard::ecs::ArchetypeData*>::Entry;

        core::memory::stack_allocator<256> indices_alloc;
        core::pod::Array<uint32_t> indices{ indices_alloc };
        core::pod::array::reserve(indices, 256 / sizeof(uint32_t));

        for (Entry const& entry : _archetype_data)
        {
            iceshard::ecs::ArchetypeData* archetype = entry.value;

            core::pod::array::clear(indices);
            if (detail::query_component_indices(archetype->components, components, indices))
            {
                uint32_t count = 0;
                iceshard::ComponentBlock* block = archetype->block;
                while(block != nullptr)
                {
                    if (block->_entity_count > 0)
                    {
                        core::pod::array::push_back(blocks, block);
                        count += 1;
                    }
                    block = block->_next;
                }

                // Push the number of blocks we got for the next stride of offsets
                core::pod::array::push_back(block_count, count);

                // Push the offsets for the selected components
                for (uint32_t component_index : indices)
                {
                    core::pod::array::push_back(block_offsets, archetype->offsets[component_index]);
                }
            }
        }
    }

    bool IceShardArchetypeIndex::query_instance(
        iceshard::Entity entity,
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t>& block_offsets,
        iceshard::ComponentBlock*& block
    ) noexcept
    {
        auto const hash_entity = core::hash(entity);
        bool const has_archetype_set = core::pod::hash::has(_entity_archetype, hash_entity);
        if (has_archetype_set == false)
        {
            return false;
        }

        detail::ArchetypeInstance instance = core::pod::hash::get(
            _entity_archetype,
            hash_entity,
            detail::ArchetypeInstance{}
        );

        bool const contains_components = detail::query_component_indices(
            instance.archetype->components,
            components,
            block_offsets
        );

        if (contains_components)
        {
            for (auto& index : block_offsets)
            {
                index = instance.archetype->offsets[index] + instance.archetype->sizes[index] * instance.index;
            }

            block = instance.block;
        }

        return contains_components;
    }

    auto create_default_index(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc
    ) noexcept -> core::memory::unique_pointer<iceshard::ecs::ArchetypeIndex>
    {
        return core::memory::make_unique<ArchetypeIndex, IceShardArchetypeIndex>(alloc, alloc, block_alloc);
    }

} // namespace iceshard::ecs
