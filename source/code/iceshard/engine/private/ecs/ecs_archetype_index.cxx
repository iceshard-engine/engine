#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

// #TODO: Remove?
#include <numeric>

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
            ice::Span<ice::ecs::detail::QueryTypeInfo const> const& conditions,
            ice::Span<ice::StringID const> const& identifiers
        ) noexcept -> ice::u32
        {
            using QueryTypeInfo = ice::ecs::detail::QueryTypeInfo;

            ice::u32 const condition_count = ice::count(conditions);
            ice::u32 const identifier_count = ice::count(identifiers);
            ice::u32 const identifier_last_index = identifier_count - 1;

            ice::u32 condition_idx = 0;
            ice::u32 identifier_idx = 1;
            ice::u64 identifier_hash = ice::hash(identifiers[1]);

            // As long as we have something to check and we did not fail search for the next ID
            ice::u32 matched_components = 0;

            bool result = true;
            for (; (condition_idx < condition_count) && (identifier_idx < identifier_count) && result; ++condition_idx)
            {
                QueryTypeInfo const& condition = conditions[condition_idx];
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
                result &= (identifier_hash == condition_hash) || condition.is_optional;

                // We only care for properly matched components, as this will allow us to even handle fully optional queries a bit better.
                //  - support for fully optional queries is there to enable them if required at some point. However we are trying to avoid it for now.
                matched_components += (identifier_hash == condition_hash);
            }

            return result ? matched_components : 0;
        }

    } // namespace detail

    template<typename T>
    concept BasicArchetypeInfo = requires(T t) {
        { t.component_identifiers } -> std::convertible_to<ice::Span<ice::StringID const>>;
        { t.component_sizes } -> std::convertible_to<ice::Span<ice::u32 const>>;
        { t.component_alignments } -> std::convertible_to<ice::Span<ice::u32 const>>;
    };

    struct ArchetypeIndex::ArchetypeDataHeader
    {
        ice::ecs::Archetype archetype_identifier;
        ice::ecs::ArchetypeInstanceInfo archetype_info;
        ice::ecs::DataBlockPool* block_pool;

        static auto calculate_meminfo(
            ice::ecs::BasicArchetypeInfo auto info,
            ice::usize& offset_ids_out,
            ice::usize& offset_values_out
        ) noexcept -> ice::meminfo
        {
            ice::ucount const component_count = ice::span::count(info.component_identifiers);

            ice::meminfo result = ice::meminfo_of<ArchetypeDataHeader>;
            offset_ids_out = result += ice::meminfo_of<ice::StringID> * component_count;
            offset_values_out = result += ice::meminfo_of<ice::ucount> * component_count * 3; // 3 = size, alignment, offset
            return result;
        }

        static auto calculate_meminfo(ice::ecs::BasicArchetypeInfo auto info) noexcept -> ice::usize
        {
            ice::usize temp;
            return calculate_meminfo(info, temp, temp).size;
        }
    };

    static constexpr ice::u32 Constant_TotalMemoryUsedForArchetypeHeaders_KiB = ice::ecs::Constant_MaxArchetypeCount * sizeof(void*) / 1024;

    ArchetypeIndex::ArchetypeIndex(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _default_block_pool{ _allocator }
        , _archetype_index{ _allocator }
        , _archetype_data{ _allocator }
    {
        ice::array::reserve(_archetype_data, ice::ecs::Constant_MaxArchetypeCount);
        ice::array::push_back(_archetype_data, nullptr);
    }

    ArchetypeIndex::~ArchetypeIndex() noexcept
    {
        for (ArchetypeDataHeader* header : ice::array::slice(_archetype_data, 1))
        {
            ice::usize size = ArchetypeDataHeader::calculate_meminfo(header->archetype_info);
            _allocator.deallocate(
                Memory{
                    .location = header,
                    .size = size,
                    .alignment = ice::align_of<ArchetypeDataHeader>
                }
            );
        }
    }

    auto ArchetypeIndex::registered_archetype_count() const noexcept -> ice::u32
    {
        return ice::array::count(_archetype_data);
    }

    auto ArchetypeIndex::register_archetype(
        ice::ecs::ArchetypeInfo const& archetype_info,
        ice::ecs::DataBlockPool* data_block_pool
    ) noexcept -> ice::ecs::Archetype
    {
        if (ice::hashmap::has(_archetype_index, ice::hash(archetype_info.identifier)))
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

        ice::u32 const component_count = ice::count(archetype_info.component_identifiers);

        ICE_ASSERT(
            component_count >= 2,
            "You cannot register an archetype with no component types!"
        );

        ice::usize offset_ids;
        ice::usize offset_values;
        ice::meminfo const header_meminfo = ArchetypeDataHeader::calculate_meminfo(
            archetype_info,
            offset_ids,
            offset_values
        );

        // We calculate the required space to store all information related to the current archetype.
        //  We need enough space for every components: identifier, size, alignment, offset
        ice::Memory const result = _allocator.allocate(header_meminfo);
        ArchetypeDataHeader* const data_header = reinterpret_cast<ArchetypeDataHeader*>(result.location);

        ice::Memory mem_component_data = ice::ptr_add(result, offset_ids);
        ice::Memory mem_component_data_end = ice::ptr_add(result, header_meminfo.size);

        // Helper variables to access all component related values
        ice::StringID const* component_identifiers = nullptr;
        ice::u32 const* component_sizes = nullptr;
        ice::u32 const* component_alignments = nullptr;
        ice::u32* component_offsets = nullptr;

        {
            // Copy the component idnetifiers
            ice::memcpy(mem_component_data, ice::span::data_view(archetype_info.component_identifiers));
            component_identifiers = reinterpret_cast<ice::StringID const*>(mem_component_data.location);

            // Calculate where we start storing the u32 values...
            mem_component_data = ice::ptr_add(result, offset_values);

            // Copy the size and alignment.
            component_sizes = reinterpret_cast<ice::u32 const*>(mem_component_data.location);
            ice::memcpy(mem_component_data, ice::span::data_view(archetype_info.component_sizes));
            mem_component_data = ice::ptr_add(mem_component_data, ice::span::size_bytes(archetype_info.component_sizes));

            component_alignments = reinterpret_cast<ice::u32 const*>(mem_component_data.location);
            ice::memcpy(mem_component_data, ice::span::data_view(archetype_info.component_alignments));
            mem_component_data = ice::ptr_add(mem_component_data, ice::span::size_bytes(archetype_info.component_alignments));

            // Save location to store calculated offsets
            component_offsets = reinterpret_cast<ice::u32*>(mem_component_data.location);
            mem_component_data = ice::ptr_add(mem_component_data, { component_count * sizeof(ice::u32) });

            ICE_ASSERT(
                mem_component_data.location == mem_component_data_end.location,
                "Copying the component info did move past the reserved space!"
            );
        }

        data_header->archetype_identifier = archetype_info.identifier;
        data_header->archetype_info.component_identifiers = ice::Span<ice::StringID const>{ component_identifiers, component_count };
        data_header->archetype_info.component_sizes = ice::Span<ice::u32 const>{ component_sizes, component_count };
        data_header->archetype_info.component_alignments = ice::Span<ice::u32 const>{ component_alignments, component_count };
        data_header->archetype_info.component_offsets = ice::Span<ice::u32 const>{ component_offsets, component_count };
        data_header->block_pool = data_block_pool;

        //// Move the block data pointer so we can store all archetype related information before it.
        ////  We also need to update the size so we properly calculate the number of entities we can still keep in this block.
        //data_block->block_data = ice::memory::ptr_align_forward(data_component_info_end, alignof(ice::ecs::EntityHandle));
        //data_block->block_data_size -= ice::memory::ptr_distance(data_header, data_block->block_data);
        //data_block->block_entity_count = 0;
        //data_block->block_entity_count_max = 0;

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

            ice::usize const block_size = data_block_pool->provided_block_size();
            ice::usize const available_block_size = { block_size.value - component_alignment_sum };

            data_header->archetype_info.component_entity_count_max = ice::ucount(available_block_size.value / component_size_sum);

            ice::u32 next_component_offset = 0;
            for (ice::u32 idx = 0; idx < component_count; ++idx)
            {
                // TAG Components have size and alignment set to '0', handle differently
                if (component_alignments[idx] == 0)
                {
                    ICE_ASSERT(
                        component_sizes[idx] == 0,
                        "Invalid component information! This '{}' seems not to be a valid tag component!",
                        ice::stringid_hint(component_identifiers[idx])
                    );

                    component_offsets[idx] = ice::u32_max;
                }
                else
                {
                    next_component_offset = ice::ecs::detail::align_forward_u32(next_component_offset, component_alignments[idx]);
                    component_offsets[idx] = next_component_offset;

                    next_component_offset += component_sizes[idx] * data_header->archetype_info.component_entity_count_max;
                }
            }
        }

        ice::u32 const archetype_index = ice::array::count(_archetype_data);
        data_header->archetype_info.archetype_instance = ArchetypeInstance{ archetype_index };

        ice::array::push_back(_archetype_data, data_header);
        ice::hashmap::set(_archetype_index, ice::hash(archetype_info.identifier), archetype_index);

        return archetype_info.identifier;
    }

    void ArchetypeIndex::find_archetypes(
        ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
        ice::Array<ice::ecs::Archetype>& out_archetypes
    ) const noexcept
    {
        ice::array::clear(out_archetypes);

        // We need to skip the first query entry if it's for `ice::ecs::EntityHandle`
        // This is due to the fact that it's always there and is not taken into account when sorting components by identifiers.
        if (ice::span::front(query_info).identifier == ice::ecs::Constant_ComponentIdentifier<ice::ecs::EntityHandle>)
        {
            query_info = ice::span::subspan(query_info, 1);
        }

        ice::u32 const required_component_count = [](auto const& query_conditions) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::detail::QueryTypeInfo const& condition : query_conditions)
            {
                result += ice::u32{ condition.is_optional == false };
            }
            return result;
        }(query_info);

        ICE_ASSERT(
            required_component_count != 0,
            "An query without any required component might impact performance as it will check every archetype possible for a match."
        );

        for (ArchetypeDataHeader const* entry : ice::array::slice(_archetype_data, 1))
        {
            ArchetypeInstanceInfo const& archetype_info = entry->archetype_info;

            ice::u32 const archetype_component_count = ice::count(archetype_info.component_identifiers);
            if (archetype_component_count < required_component_count)
            {
                continue;
            }

            ice::u32 const matched_components = ice::ecs::detail::contains_required_components(
                query_info,
                archetype_info.component_identifiers
            );

            // If we don't match any component in a full optional query, we still skip this archetype.
            //  #todo: we should probably also check for the existance of the EntityHandle in the query. Then the check should be `> 1`
            if (matched_components > 0)
            {
                ice::array::push_back(out_archetypes, entry->archetype_identifier);
            }
        }
    }

    void ArchetypeIndex::fetch_archetype_instance_infos(
        ice::Span<ice::ecs::Archetype const> archetypes,
        ice::Span<ice::ecs::ArchetypeInstanceInfo const*> out_instance_infos
    ) const noexcept
    {
        ICE_ASSERT(
            ice::count(archetypes) == ice::count(out_instance_infos),
            "Archetype instance fetch called with different input and output array sizes."
        );

        ice::u32 const instance_count = ice::array::count(_archetype_data);

        ice::u32 archetype_idx = 0;
        for (Archetype archetype : archetypes)
        {
            ice::u32 const instance_idx = ice::hashmap::get(_archetype_index, ice::hash(archetype), ice::u32_max);
            ICE_ASSERT(
                instance_idx < instance_count,
                "Unknown archetype handle {} provided while fetching instance infos. Did you forget to register this archetype?",
                ice::hash(archetype)
            );

            out_instance_infos[archetype_idx] = ice::addressof(_archetype_data[instance_idx]->archetype_info);
            archetype_idx += 1;
        }

    }

    void ArchetypeIndex::fetch_archetype_instance_infos(
        ice::Span<ice::ecs::ArchetypeInstance const> archetype_instances,
        ice::Span<ice::ecs::ArchetypeInstanceInfo const*> out_instance_infos
    ) const noexcept
    {
        ICE_ASSERT(
            ice::count(archetype_instances) == ice::count(out_instance_infos),
            "Archetype instance fetch called with different input and output array sizes."
        );

        ice::u32 const instance_count = ice::array::count(_archetype_data);

        ice::u32 archetype_idx = 0;
        for (ArchetypeInstance archetype_instance : archetype_instances)
        {
            ice::u32 const instance_idx = static_cast<ice::u32>(archetype_instance);
            ICE_ASSERT(
                instance_idx < instance_count,
                "Provided archetype instance {} is invalid!",
                ice::hash(archetype_instance)
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
        ice::u32 const instance_count = ice::array::count(_archetype_data);
        ice::u32 const instance_idx = ice::hashmap::get(_archetype_index, ice::hash(archetype), ice::u32_max);

        if (instance_idx < instance_count)
        {
            ArchetypeDataHeader const* const header = _archetype_data[instance_idx];

            out_instance_info = ice::addressof(header->archetype_info);
            out_block_pool = header->block_pool;
        }
        else
        {
            out_instance_info = nullptr;
            out_block_pool = nullptr;
        }
    }

    void ArchetypeIndex::fetch_archetype_instance_pool(
        ice::ecs::ArchetypeInstance archetype_instance,
        ice::ecs::DataBlockPool*& out_block_pool
    ) const noexcept
    {
        ice::u32 const instance_count = ice::array::count(_archetype_data);
        ice::u32 const instance_idx = static_cast<ice::u32>(archetype_instance);

        if (instance_idx < instance_count)
        {
            ArchetypeDataHeader const* const header = _archetype_data[instance_idx];
            out_block_pool = header->block_pool;
        }
        else
        {
            out_block_pool = nullptr;
        }
    }

} // ice::ecs
