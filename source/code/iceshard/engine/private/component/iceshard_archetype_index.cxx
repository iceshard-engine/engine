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
                    if (src_archetype_data->components[main_component_index] != dst_archetype_data->components[sub_component_index])
                    {
                        continue;
                    }

                    IS_ASSERT(
                        src_archetype_data->sizes[main_component_index] == dst_archetype_data->sizes[sub_component_index],
                        "Mismatched data size {} != {} for components with the same ID. source: {}, destination: {}",
                        src_archetype_data->sizes[main_component_index], dst_archetype_data->sizes[sub_component_index],
                        src_archetype_data->components[main_component_index], dst_archetype_data->components[sub_component_index]
                    );

                    uint32_t const size = src_archetype_data->sizes[main_component_index];
                    uint32_t const src_offset = src_archetype_data->offsets[main_component_index] + size * operation.src_index;
                    uint32_t const dst_offset = dst_archetype_data->offsets[sub_component_index] + size * operation.dst_index;

                    void* const src_ptr = core::memory::utils::pointer_add(operation.src_block, src_offset);
                    void* const dst_ptr = core::memory::utils::pointer_add(operation.dst_block, dst_offset);

                    // Do a plain copy (we require components to be standard layout and trivially copyable)
                    memcpy(dst_ptr, src_ptr, size);

                    // Increment the sub index
                    sub_component_index += 1;
                }
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
                block = block->_next;

                iceshard::detail::component_block_prepare(
                    block,
                    core::pod::array::begin(archetype_data->sizes),
                    core::pod::array::begin(archetype_data->alignments),
                    core::pod::array::size(archetype_data->components),
                    core::pod::array::begin(archetype_data->offsets)
                );

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
        auto base_size = detail::single_value_array<uint32_t>(4u, temp_alloc);
        auto base_alignment = detail::single_value_array<uint32_t>(4u, temp_alloc);

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

        detail::ArchetypeDataOperation copy_op;
        copy_op.src_archetype = src_instance.archetype;
        copy_op.src_block = src_instance.block;
        copy_op.src_index = src_instance.index;
        copy_op.dst_archetype = dst_instance.archetype;
        copy_op.dst_block = dst_instance.block;
        copy_op.dst_index = dst_instance.index;
        copy_op.count = 1;

        if (copy_op.src_block != nullptr)
        {
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

    auto create_default_index(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc
    ) noexcept -> core::memory::unique_pointer<iceshard::ecs::ArchetypeIndex>
    {
        return core::memory::make_unique<ArchetypeIndex, IceShardArchetypeIndex>(alloc, alloc, block_alloc);
    }

} // namespace iceshard::ecs
