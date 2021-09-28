#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/pod/array.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

namespace ice::ecs
{

    namespace detail
    {

        constexpr auto align_forward_u32(
            ice::u32 base_offset,
            ice::u32 alignment
        ) noexcept -> ice::u32
        {
            ice::u32 const alignment_delta = base_offset % alignment;
            if (alignment_delta > 0)
            {
                base_offset += (alignment - alignment_delta);
            }
            return base_offset;
        }

        auto contains_required_components(
            ice::Span<ice::ecs::ArchetypeQuery::QueryCondition const> const& conditions,
            ice::Span<ice::StringID const> const& identifiers
        ) noexcept -> ice::u32
        {
            using Condition = ArchetypeQuery::QueryCondition;

            ice::u32 const condition_count = ice::size(conditions);
            ice::u32 const identifier_count = ice::size(identifiers);
            ice::u32 const identifier_last_index = identifier_count - 1;

            ice::u32 condition_idx = 0;
            ice::u32 identifier_idx = 0;
            ice::u64 identifier_hash = ice::hash(identifiers[0]);

            // As long as we have something to check and we did not fail search for the next ID
            ice::u32 matched_components = 0;

            bool result = true;
            for (; (condition_idx < condition_count) && (identifier_idx < identifier_count) && result; ++condition_idx)
            {
                Condition const& condition = conditions[condition_idx];
                ice::u64 const condition_hash = ice::hash(condition.identifier);

                // As long as `condition_hash` is greater than `identifier_hash` check the next identifier.
                while (condition_hash > identifier_hash && identifier_idx < identifier_last_index)
                {
                    identifier_idx += 1;
                    identifier_hash = ice::hash(identifiers[identifier_idx]);
                }

                // We are either equal or smaller
                //  - if smaller, then there is not a single value left in `identifiers` that would match.
                //  - if we failed to match, we can still check if the condition was optional
                result &= (identifier_hash == condition_hash) || condition.optional;

                // We only care for properly matched components, as this will allow us to even handle fully optional queries a bit better.
                //  - support for fully optional queries is there to enable them if required at some point. However we are trying to avoid it for now.
                matched_components += (identifier_hash == condition_hash);
            }

            return result ? matched_components : 0;
        }

    } // namespace detail

    struct ArchetypeIndex::ArchetypeDataHeader
    {
        ice::ecs::Archetype archetype_identifier;
        ice::ecs::ArchetypeInstanceInfo archetype_info;
        ice::ecs::DataBlockPool* block_pool;
        ice::ecs::DataBlock* first_block;
    };

    static constexpr ice::u32 Constant_TotalMemoryUsedForArchetypeHeaders_KiB = ice::ecs::Constant_MaxArchetypeCount * sizeof(void*) / 1024;

    ArchetypeIndex::ArchetypeIndex(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _default_block_pool{ _allocator }
        , _archetype_index{ _allocator }
        , _archetype_data{ _allocator }
    {
        ice::pod::array::reserve(_archetype_data, ice::ecs::Constant_MaxArchetypeCount);
    }

    auto ArchetypeIndex::register_archetype(
        ice::ecs::ArchetypeInfo const& archetype_info,
        ice::ecs::DataBlockPool* data_block_pool
    ) noexcept -> ice::ecs::Archetype
    {
        if (ice::pod::hash::has(_archetype_index, ice::hash(archetype_info.identifier)))
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "This archetype {} is already registered!",
                ice::hash32(archetype_info.identifier)
            );
            return archetype_info.identifier;
        }

        if (data_block_pool == nullptr)
        {
            data_block_pool = &_default_block_pool;
        }

        ice::u32 const component_count = ice::size(archetype_info.component_identifiers);

        // We calculate the required space to store all information related to the current archetype.
        //  We need enough space for every components: identifier, size, alignment, offset
        ice::u32 const component_info_byte_sum = component_count * sizeof(ice::StringID)
            + component_count * sizeof(ice::u32) * 3;

        DataBlock* const data_block = data_block_pool->request_block();
        ArchetypeDataHeader* const data_header = reinterpret_cast<ArchetypeDataHeader*>(data_block->block_data);

        void* data_component_info = ice::memory::ptr_align_forward(data_header + 1, alignof(ice::StringID));
        void* const data_component_info_end = ice::memory::ptr_add(data_component_info, component_info_byte_sum);

        // Helper variables to access all component related values
        ice::StringID const* component_identifiers = nullptr;
        ice::u32 const* component_sizes = nullptr;
        ice::u32 const* component_alignments = nullptr;
        ice::u32* component_offsets = nullptr;

        {
            // Copy the component idnetifiers
            ice::memcpy(data_component_info, archetype_info.component_identifiers.data(), archetype_info.component_identifiers.size_bytes());
            component_identifiers = reinterpret_cast<ice::StringID const*>(data_component_info);
            data_component_info = ice::memory::ptr_add(data_component_info, archetype_info.component_identifiers.size_bytes());

            // Copy the size and alignment.
            ice::memcpy(data_component_info, archetype_info.component_sizes.data(), archetype_info.component_sizes.size_bytes());
            component_sizes = reinterpret_cast<ice::u32 const*>(data_component_info);
            data_component_info = ice::memory::ptr_add(data_component_info, archetype_info.component_sizes.size_bytes());

            ice::memcpy(data_component_info, archetype_info.component_alignments.data(), archetype_info.component_alignments.size_bytes());
            component_alignments = reinterpret_cast<ice::u32 const*>(data_component_info);
            data_component_info = ice::memory::ptr_add(data_component_info, archetype_info.component_alignments.size_bytes());

            // Save location to store calculated offsets
            component_offsets = reinterpret_cast<ice::u32*>(data_component_info);
            data_component_info = ice::memory::ptr_add(data_component_info, component_count * sizeof(ice::u32));

            ICE_ASSERT(data_component_info == data_component_info_end, "Copying the component info did move past the reserved space!");
        }

        data_header->archetype_identifier = archetype_info.identifier;
        data_header->archetype_info.component_identifiers = ice::Span<ice::StringID const>{ component_identifiers, component_count };
        data_header->archetype_info.component_sizes = ice::Span<ice::u32 const>{ component_sizes, component_count };
        data_header->archetype_info.component_alignments = ice::Span<ice::u32 const>{ component_alignments, component_count };
        data_header->archetype_info.component_offsets = ice::Span<ice::u32 const>{ component_offsets, component_count };
        data_header->block_pool = data_block_pool;
        data_header->first_block = data_block;

        // Move the block data pointer so we can store all archetype related information before it.
        //  We also need to update the size so we properly calculate the number of entities we can still keep in this block.
        data_block->block_data = ice::memory::ptr_align_forward(data_component_info_end, alignof(ice::ecs::EntityHandle));
        data_block->block_data_size -= ice::memory::ptr_distance(data_header, data_block->block_data);
        data_block->block_entity_count = 0;
        data_block->block_entity_count_max = 0;

        // We need now to calculate the number of entities that we can store in the remaining memory.
        //  Additionally calculate the offets each component array will be located at.
        {
            ice::u32 const component_size_sum = std::accumulate(
                component_sizes,
                component_sizes + component_count,
                0
            );

            ice::u32 const component_alignment_sum = std::accumulate(
                component_alignments,
                component_alignments + component_count,
                0
            );

            // #todo: Check if this ever will be again required with the current approach.
            //ice::u32 max_alignment = component_alignments[0];
            //for (ice::u32 idx = 1; idx < component_count; ++idx)
            //{
            //    max_alignment = ice::max(component_alignments[idx], max_alignment);
            //}

            ice::u32 const block_size = data_block->block_data_size;
            ice::u32 const available_block_size = block_size - component_alignment_sum; // (component_alignment_sum + max_alignment);

            data_block->block_entity_count_max = available_block_size / component_size_sum;

            ice::u32 next_component_offset = 0;
            for (ice::u32 idx = 0; idx < component_count; ++idx)
            {
                next_component_offset = ice::ecs::detail::align_forward_u32(next_component_offset, component_alignments[idx]);
                component_offsets[idx] = next_component_offset;

                next_component_offset = component_sizes[idx] * data_block->block_entity_count_max;
            }
        }

        ice::u32 const archetype_index = ice::pod::array::size(_archetype_data);
        ice::pod::array::push_back(_archetype_data, data_header);
        ice::pod::hash::set(_archetype_index, ice::hash(archetype_info.identifier), archetype_index);

        return archetype_info.identifier;
    }

    void ArchetypeIndex::find_archetypes(
        ice::ecs::ArchetypeQuery const& query_info,
        ice::pod::Array<ice::ecs::Archetype>& out_archetypes
    ) const noexcept
    {
        ice::pod::array::clear(out_archetypes);

        ice::u32 const query_condition_count = ice::size(query_info.query_conditions);
        ice::u32 const required_component_count = [](auto const& query_conditions) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::ArchetypeQuery::QueryCondition const& condition : query_conditions)
            {
                result += ice::u32{ condition.optional == false };
            }
            return result;
        }(query_info.query_conditions);

        ICE_ASSERT(
            required_component_count == 0,
            "An query without any required component might impact performance as it will check every archetype possible for a match."
        );

        for (ArchetypeDataHeader const* entry : _archetype_data)
        {
            ArchetypeInstanceInfo const& archetype_info = entry->archetype_info;

            ice::u32 const archetype_component_count = ice::size(archetype_info.component_identifiers);
            if (archetype_component_count < required_component_count)
            {
                continue;
            }

            ice::u32 const matched_components = ice::ecs::detail::contains_required_components(
                query_info.query_conditions,
                archetype_info.component_identifiers
            );

            // If we don't match any component in a full optional query, we still skip this archetype.
            //  #todo: we should probably also check for the existance of the EntityHandle in the query. Then the check should be `> 1`
            if (matched_components > 0)
            {
                ice::pod::array::push_back(out_archetypes, entry->archetype_identifier);
            }
        }
    }

    void ArchetypeIndex::fetch_archetype_instance_infos(
        ice::Span<ice::ecs::Archetype const> archetypes,
        ice::Span<ice::ecs::ArchetypeInstanceInfo const*> out_instance_infos
    ) const noexcept
    {
        ice::u32 const instance_count = ice::pod::array::size(_archetype_data);

        ice::u32 archetype_idx = 0;
        for (Archetype archetype : archetypes)
        {
            ice::u32 const instance_idx = ice::pod::hash::get(_archetype_index, ice::hash(archetype), ice::u32_max);
            ICE_ASSERT(
                instance_idx < instance_count,
                "Unknown archetype handle {} provided while fetching instance infos. Did you forget to register this archetype?",
                ice::hash(archetype)
            );

            out_instance_infos[archetype_idx] = ice::addressof(_archetype_data[instance_idx]->archetype_info);
        }

    }

    void ArchetypeIndex::fetch_archetype_instance_info_with_pool(
        ice::ecs::Archetype archetype,
        ice::ecs::ArchetypeInstanceInfo const*& out_instance_info,
        ice::ecs::DataBlockPool*& out_block_pool
    ) const noexcept
    {
        ice::u32 const instance_count = ice::pod::array::size(_archetype_data);
        ice::u32 const instance_idx = ice::pod::hash::get(_archetype_index, ice::hash(archetype), ice::u32_max);

        ICE_ASSERT(
            instance_idx < instance_count,
            "Unknown archetype handle {} provided while fetching instance info with block pool. Did you forget to register the archetype?",
            ice::hash(archetype)
        );

        ArchetypeDataHeader const* const header = _archetype_data[instance_idx];

        out_instance_info = ice::addressof(header->archetype_info);
        out_block_pool = header->block_pool;
    }

} // ice::ecs
