#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/pod/hash.hxx>

#include <core/allocators/stack_allocator.hxx>
#include <iceshard/component/component_block_operation.hxx>

namespace iceshard
{
    namespace detail
    {

        auto hash_mix(uint64_t left, uint64_t right) noexcept
        {
            left ^= (left >> 33);
            left ^= right;
            left *= 0xff51afd7ed558ccd;
            left ^= (left >> 33);
            left ^= right;
            left *= 0xc4ceb9fe1a85ec53;
            left ^= (left >> 33);
            return left;
        }

        auto make_identifier(uint64_t prototype_hash) noexcept -> core::stringid_type
        {
            return { core::cexpr::stringid_hash_type{ prototype_hash } };
        }

    } // namespace detail

    struct ArchetypeInfo
    {
        core::stringid_type identifier;
        core::stringid_type parent_archetype = core::stringid_invalid;
    };

    struct ArchetypeData
    {
        ArchetypeData(core::allocator& alloc, iceshard::ComponentBlockAllocator* block_alloc) noexcept
            : sizes{ alloc }
            , components{ alloc }
            , alignments{ alloc }
            , offsets{ alloc }
            , block_allocator{ block_alloc }
        {
            core::pod::array::push_back(components, "isc.entity"_sid);
            core::pod::array::push_back(sizes, static_cast<uint32_t>(sizeof(Entity)));
            core::pod::array::push_back(alignments, static_cast<uint32_t>(alignof(Entity)));
        }

        ~ArchetypeData() noexcept = default;

        core::pod::Array<core::stringid_type> components;
        core::pod::Array<uint32_t> sizes;
        core::pod::Array<uint32_t> alignments;
        core::pod::Array<uint32_t> offsets;

        iceshard::ComponentBlockAllocator* block_allocator;
        iceshard::ComponentBlock* block = nullptr;

        uint32_t block_count = 0;
    };

    struct ArchetypeInstance
    {
        core::stringid_type archetype;
        iceshard::ComponentBlock* block;
        uint32_t index = 0;
    };

    class ComponentArchetypeIndex
    {
    public:
        ComponentArchetypeIndex(core::allocator& alloc, iceshard::ComponentBlockAllocator* block_alloc) noexcept
            : _allocator{ alloc }
            , _block_allocator{ block_alloc }
            , _entity_archetype{ _allocator }
            , _archetype_info{ _allocator }
            , _archetype_data{ _allocator }
        {
            core::pod::hash::set(_archetype_info, core::hash(core::stringid_invalid), ArchetypeInfo{ core::stringid_invalid, core::stringid_invalid });

            ArchetypeData* empty_data = _allocator.make<ArchetypeData>(_allocator, _block_allocator);
            core::pod::hash::set(_archetype_data, core::hash(core::stringid_invalid), empty_data);
        }

        ~ComponentArchetypeIndex() noexcept
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

        auto get_archetype(core::pod::Array<core::stringid_type> const& component_names) const noexcept -> core::stringid_type
        {
            auto archetype_id = core::hash(core::stringid_invalid);

            // Iterate over component identifier names and create the final archetype id.
            for (auto component_name : component_names)
            {
                archetype_id = detail::hash_mix(archetype_id, core::hash(component_name));
            }

            return detail::make_identifier(archetype_id);
        }

        auto get_or_create_archetype(
            core::stringid_type parent_archetype,
            core::stringid_type component_name,
            uint32_t size,
            uint32_t alignment
        ) noexcept -> core::stringid_type
        {
            uint64_t parent_archetype_hash = core::hash(parent_archetype);

            IS_ASSERT(core::pod::hash::has(_archetype_info, parent_archetype_hash), "Base archetype {} does not exist!", parent_archetype);

            uint64_t archetype_hash = detail::hash_mix(parent_archetype_hash, core::hash(component_name));

            // If an archetype already exists, just return the identifier
            if (core::pod::hash::has(_archetype_info, archetype_hash))
            {
                return detail::make_identifier(archetype_hash);
            }

            ArchetypeData* parent_archetype_data = core::pod::hash::get<ArchetypeData*>(
                _archetype_data,
                parent_archetype_hash,
                nullptr
            );

            // Create the new archetype based on the parent
            ArchetypeData* archetype_data = _allocator.make<ArchetypeData>(
                _allocator,
                _block_allocator
            );

            archetype_data->components = parent_archetype_data->components;
            archetype_data->sizes = parent_archetype_data->sizes;
            archetype_data->alignments = parent_archetype_data->alignments;

            archetype_data->block = archetype_data->block_allocator->alloc_block();
            archetype_data->block->_entity_count = 0;
            archetype_data->block_count = 1;

            core::pod::array::push_back(archetype_data->components, component_name);
            core::pod::array::push_back(archetype_data->sizes, size);
            core::pod::array::push_back(archetype_data->alignments, alignment);

            {
                auto const component_count = core::pod::array::size(archetype_data->sizes);
                core::pod::array::resize(archetype_data->offsets, component_count);

                iceshard::detail::component_block_prepare(
                    archetype_data->block,
                    core::pod::array::begin(archetype_data->sizes),
                    core::pod::array::begin(archetype_data->alignments),
                    component_count,
                    core::pod::array::begin(archetype_data->offsets)
                );
            }

            ArchetypeInfo archetype_info;
            archetype_info.identifier = component_name;
            archetype_info.parent_archetype = parent_archetype;

            core::pod::hash::set(_archetype_info, archetype_hash, archetype_info);
            core::pod::hash::set(_archetype_data, archetype_hash, archetype_data);

            return detail::make_identifier(archetype_hash);
        }

        void add_component(
            Entity entity,
            core::stringid_arg_type component_name,
            uint32_t size,
            uint32_t alignment
        ) noexcept
        {
            ArchetypeInstance entity_instance = core::pod::hash::get(
                _entity_archetype,
                core::hash(entity),
                ArchetypeInstance{ core::stringid_invalid }
            );

            core::stringid_type new_archetype = get_or_create_archetype(
                entity_instance.archetype,
                component_name,
                size,
                alignment
            );

            ArchetypeData* archetype_data = core::pod::hash::get(
                _archetype_data,
                core::hash(new_archetype),
                nullptr
            );

            ComponentBlock* block = archetype_data->block;
            if ((block->_entity_count + 1) > block->_entity_count_max)
            {
                archetype_data->block = _block_allocator->alloc_block();
                archetype_data->block->_next = block;
                archetype_data->block_count += 1;

                block = archetype_data->block;
            }

            // Helpers
            void* src_block_ptr = entity_instance.block;
            void* dest_block_ptr = block;

            if (src_block_ptr)
            {
                ArchetypeData* src_archetype_data = core::pod::hash::get(
                    _archetype_data,
                    core::hash(entity_instance.archetype),
                    nullptr
                );

                // Iterate over each component in the source archetype
                uint32_t const component_count = core::pod::array::size(src_archetype_data->components);
                for (uint32_t component_idx = 0; component_idx < component_count; ++component_idx)
                {
                    uint32_t const component_size = src_archetype_data->sizes[component_idx];
                    IS_ASSERT(
                        component_size == archetype_data->sizes[component_idx],
                        "Component sizes ({} != {}) do not match! Archetype fatal mismatch while adding component {} to entity {}!",
                        component_size, archetype_data->sizes[component_idx],
                        component_name,
                        core::hash(entity)
                    );

                    uint32_t const src_offset = src_archetype_data->offsets[component_idx] + component_size * entity_instance.index;
                    uint32_t const dest_offset = archetype_data->offsets[component_idx] + component_size * block->_entity_count;

                    void* const src_ptr = core::memory::utils::pointer_add(src_block_ptr, src_offset);
                    void* const dest_ptr = core::memory::utils::pointer_add(dest_block_ptr, dest_offset);

                    // Do a plain copy (we require components to be standard layout and trivially copyable)
                    memcpy(dest_ptr, src_ptr, component_size);
                }
            }
            else
            {
                // Update the `isc.entity` component
                void* const entity_data_ptr = core::memory::utils::pointer_add(block, archetype_data->offsets[0] + sizeof(Entity) * block->_entity_count);
                Entity* const entity_component_ptr = reinterpret_cast<Entity*>(entity_data_ptr);
                *entity_component_ptr = entity;
            }

            core::pod::hash::set(_entity_archetype, core::hash(entity), ArchetypeInstance{
                new_archetype,
                block,
                block->_entity_count
            });

            block->_entity_count += 1;
        }

        void query_components_archetypes(
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<core::stringid_type>& matching_archetypes,
            core::pod::Array<uint32_t>& component_indexes
        ) noexcept
        {
            uint32_t component_array_size = 0;

            for (core::pod::Hash<ArchetypeData*>::Entry const& entry : _archetype_data)
            {
                ArchetypeData* const archetype_data = entry.value;
                core::pod::Array<core::stringid_type> const& prototype_components = archetype_data->components;

                uint32_t required_components_count = core::pod::array::size(required_components);
                uint32_t prototype_component_count = core::pod::array::size(prototype_components);

                if (required_components_count > prototype_component_count)
                {
                    continue;
                }

                bool has_component = true;
                for (uint32_t idx = 0; idx < required_components_count && has_component; ++idx)
                {

                    has_component = false;
                    for (uint32_t proto_idx = 0; proto_idx < prototype_component_count && !has_component; ++proto_idx)
                    {
                        if (required_components[idx] == prototype_components[proto_idx])
                        {
                            has_component = true;
                            core::pod::array::push_back(component_indexes, proto_idx);
                        }
                    }

                }

                if (has_component)
                {
                    core::pod::array::push_back(matching_archetypes, detail::make_identifier(entry.key));
                    component_array_size = core::pod::array::size(component_indexes);
                }
                else
                {
                    core::pod::array::resize(component_indexes, component_array_size);
                }
            }
        }

        void query_components_archetypes(
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<core::stringid_type>& result_archetypes
        ) noexcept
        {
            auto contains_components = [&required_components](core::pod::Array<core::stringid_type> const& prototype_components) noexcept
            {
                uint32_t required_components_count = core::pod::array::size(required_components);
                uint32_t prototype_component_count = core::pod::array::size(prototype_components);

                if (required_components_count > prototype_component_count)
                {
                    return false;
                }

                bool proto_has_component = true;
                for (uint32_t idx = 0; idx < required_components_count && proto_has_component; ++idx)
                {
                    proto_has_component = false;
                    for (uint32_t proto_idx = 0; proto_idx < prototype_component_count && !proto_has_component; ++proto_idx)
                    {
                        proto_has_component = required_components[idx] == prototype_components[proto_idx];
                    }
                }

                return proto_has_component;
            };

            for (core::pod::Hash<ArchetypeData*>::Entry const& entry : _archetype_data)
            {
                ArchetypeData const* archetype_info = entry.value;

                if (contains_components(archetype_info->components))
                {
                    core::pod::array::push_back(result_archetypes, detail::make_identifier(entry.key));
                }
            }
        }

    protected:
        static bool find_all_components(
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<core::stringid_type> const& available_components,
            core::pod::Array<uint32_t>& component_indexes
        ) noexcept
        {
            uint32_t required_components_count = core::pod::array::size(required_components);
            uint32_t available_component_count = core::pod::array::size(available_components);

            if (required_components_count > available_component_count)
            {
                return false;
            }

            for (uint32_t idx = 0; idx < required_components_count; ++idx)
            {
                for (uint32_t proto_idx = 0; proto_idx < available_component_count; ++proto_idx)
                {
                    if (required_components[idx] == available_components[proto_idx])
                    {
                        core::pod::array::push_back(component_indexes, proto_idx);
                    }
                }
            }

            return core::pod::array::size(component_indexes) == required_components_count;
        }

        void select_archetypes_components(
            core::pod::Array<core::stringid_type> const& required_components,
            //core::pod::Array<core::stringid_type> const& archetypes,
            core::pod::Array<ArchetypeData*>& archetypes_data,
            core::pod::Array<uint32_t>& component_indices
        ) noexcept
        {
            // helper array to store indexes of selected components
            core::memory::stack_allocator<128> indices_allocator{ };
            core::pod::Array<uint32_t> indices{ indices_allocator };

            // Check each archetype if it contains the required components and save the archetype data and indexes of these components.
            for (core::pod::Hash<ArchetypeData*>::Entry const& entry : _archetype_data)
            {
                ArchetypeData* const archetype_data = entry.value;

                // Prepare helper arrays for the next iteration
                core::pod::array::clear(indices);

                // If we don't find all required components in the archetype data skip this archetype.
                if (!find_all_components(required_components, archetype_data->components, indices))
                {
                    continue;
                }

                // Append to results variables
                core::pod::array::push_back(component_indices, indices);
                core::pod::array::push_back(archetypes_data, archetype_data);
            }
        }

        void select_archetype_components(
            iceshard::ArchetypeData const* archetype_data,
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<uint32_t>& component_indices
        ) noexcept
        {
            // helper array to store indexes of selected components
            core::memory::stack_allocator<128> indices_allocator{ };
            core::pod::Array<uint32_t> indices{ indices_allocator };

            // If we don't find all required components in the archetype data skip this archetype.
            if (!find_all_components(required_components, archetype_data->components, indices))
            {
                return;
            }

            // Append to results variables
            core::pod::array::push_back(component_indices, indices);
        }

    public:
        void query_components_entity(
            iceshard::Entity entity,
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<uint32_t>& offsets,
            iceshard::ComponentBlock*& block
        ) noexcept
        {
            // Find the archetype instance
            ArchetypeInstance archetype_instance = core::pod::hash::get(
                _entity_archetype,
                core::hash(entity),
                ArchetypeInstance{ core::stringid_invalid }
            );

            ArchetypeData* const archetype_data = core::pod::hash::get(
                _archetype_data,
                core::hash(archetype_instance.archetype),
                nullptr
            );

            // Select indices of required components.
            select_archetype_components(archetype_data, required_components, offsets);

            // Extract offsets and component block
            for (auto& offset : offsets)
            {
                // We replace the selected component index with it's offset
                offset = archetype_data->offsets[offset] + archetype_data->sizes[offset] * archetype_instance.index;
            }

            block = archetype_instance.block;
        }

        void query_components_instances(
            core::pod::Array<core::stringid_type> const& required_components,
            core::pod::Array<uint32_t>& offsets,
            core::pod::Array<iceshard::ComponentBlock*>& blocks,
            core::pod::Array<uint32_t>& counts
        ) noexcept
        {
            core::memory::stack_allocator<256> selected_archetypes_alloc;
            core::pod::Array<ArchetypeData*> selected_archetypes{ selected_archetypes_alloc };

            // Select indices of required components.
            select_archetypes_components(required_components, selected_archetypes, offsets);

            uint32_t const offset_stride = core::pod::array::size(required_components);
            uint32_t offset_base = 0;

            // Extract all offsets for each archetype
            for (ArchetypeData* archetype_data : selected_archetypes)
            {
                for (uint32_t idx = 0; idx < offset_stride; ++idx)
                {
                    // We replace the selected component index with it's offset
                    uint32_t offset_idx = offset_base + idx;
                    offsets[offset_idx] = archetype_data->offsets[offsets[offset_idx]];
                }

                offset_base += offset_stride;

                auto it = archetype_data->block;
                while (it != nullptr)
                {
                    core::pod::array::push_back(blocks, it);
                    it = it->_next;
                }

                core::pod::array::push_back(counts, archetype_data->block_count);
            }
        }

    private:
        core::allocator& _allocator;
        iceshard::ComponentBlockAllocator* _block_allocator;

        core::pod::Hash<ArchetypeInstance> _entity_archetype;
        core::pod::Hash<ArchetypeInfo> _archetype_info;
        core::pod::Hash<ArchetypeData*> _archetype_data;
    };

    struct ComponentBlock;

    class ComponentBlockAllocator;

    class ComponentArchetype
    {
    public:
        ComponentArchetype(ComponentBlockAllocator& alloc, core::stringid_arg_type id) noexcept
            : _allocator{ alloc }
            , _identifier{ id }
        {
        }

        ~ComponentArchetype() noexcept
        {
            while (nullptr != _block)
            {
                [[maybe_unused]]
                auto* next = _block->_next;
                // alloc.destroy(_block);
                // _block = next;
            }
        }

    private:
        ComponentBlockAllocator& _allocator;
        core::stringid_type _identifier;
        ComponentBlock* _block = nullptr;
    };

} // namespace iceshard
