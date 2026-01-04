/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

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

        bool contains_required_components(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> in_conditions,
            ice::Span<ice::StringID const> in_required_tags,
            ice::Span<ice::StringID const> checked_identifiers
        ) noexcept
        {
            using QueryTypeInfo = ice::ecs::detail::QueryTypeInfo;

            ice::u32 const tag_count = ice::count(in_required_tags);
            ice::u32 const condition_count = ice::count(in_conditions);
            ice::u32 const identifier_count = ice::count(checked_identifiers);
            ice::u32 const identifier_last_index = identifier_count - 1;

            ice::u32 tag_idx = 0;
            ice::u32 condition_idx = 0;
            // We skip the first identifier, since it's always the 'entity' ID
            ice::u32 identifier_idx = 1;
            ice::u64 identifier_hash = ice::hash(checked_identifiers[1]);

            bool result = true;
            for (; (tag_idx < tag_count) && (identifier_idx < identifier_count) && result; ++tag_idx)
            {
                ice::u64 const tag_hash = ice::hash(in_required_tags[tag_idx]);

                // As long as `tag_hash` is greater than `identifier_hash` check the next identifier.
                while (tag_hash > identifier_hash && identifier_idx < identifier_last_index)
                {
                    identifier_idx += 1;
                    identifier_hash = ice::hash(checked_identifiers[identifier_idx]);
                }

                // We only allow equality checks for tags
                //  - if we failed the whole archetypes is removed as a possible match.
                result &= (identifier_hash == tag_hash);
            }

            // As long as we have something to check and we did not fail search for the next ID
            [[maybe_unused]]
            ice::u32 matched_components = 0;

            // Reset for checking the remaining conditions
            identifier_idx = 1;
            identifier_hash = ice::hash(checked_identifiers[1]);

            for (; (condition_idx < condition_count) && (identifier_idx < identifier_count) && result; ++condition_idx)
            {
                QueryTypeInfo const& condition = in_conditions[condition_idx];
                ice::u64 const condition_hash = ice::hash(condition.identifier);

                // As long as `condition_hash` is greater than `identifier_hash` check the next identifier.
                while (condition_hash > identifier_hash && identifier_idx < identifier_last_index)
                {
                    identifier_idx += 1;
                    identifier_hash = ice::hash(checked_identifiers[identifier_idx]);
                }

                // We are either equal or smaller
                //  - if smaller, then there is not a single value left in `identifiers` that would match.
                //  - if we failed to match, we can still check if the condition was optional
                result &= (identifier_hash == condition_hash) || condition.is_optional;

                // We only care for properly matched components, as this will allow us to even handle fully optional queries a bit better.
                //  - support for fully optional queries is there to enable them if required at some point. However we are trying to avoid it for now.
                matched_components += (identifier_hash == condition_hash);
            }

            return result;
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
        ice::String archetype_name;
        ice::ecs::Archetype archetype_identifier;
        ice::ecs::detail::ArchetypeInstanceInfo archetype_info;
        ice::ecs::detail::DataBlockPool* block_pool;

        static auto calculate_meminfo(
            ice::String name,
            ice::ecs::BasicArchetypeInfo auto info,
            ice::usize& offset_name_out,
            ice::usize& offset_ids_out,
            ice::usize& offset_values_out
        ) noexcept -> ice::meminfo
        {
            ice::u32 const component_count = ice::span::count(info.component_identifiers);

            ice::meminfo result = ice::meminfo_of<ArchetypeDataHeader>;
            offset_name_out = result += ice::meminfo_of<ice::utf8> * (name.size() + 1);
            offset_ids_out = result += ice::meminfo_of<ice::StringID> * component_count;
            offset_values_out = result += ice::meminfo_of<ice::u32> * component_count * 3; // 3 = size, alignment, offset
            return result;
        }

        static auto calculate_meminfo(ice::String name, ice::ecs::BasicArchetypeInfo auto info) noexcept -> ice::usize
        {
            ice::usize temp;
            return calculate_meminfo(name, info, temp, temp, temp).size;
        }
    };

    static constexpr ice::u32 Constant_TotalMemoryUsedForArchetypeHeaders_KiB = ice::ecs::Constant_MaxArchetypeCount * sizeof(void*) / 1024;

    ArchetypeIndex::ArchetypeIndex(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _default_block_pool{ ice::ecs::detail::create_default_block_pool(_allocator) }
        , _archetype_index{ _allocator }
        , _archetype_names_index{ _allocator }
        , _archetype_data{ _allocator }
    {
        ice::array::reserve(_archetype_data, ice::ecs::Constant_MaxArchetypeCount);
        ice::array::push_back(_archetype_data, nullptr);
    }

    ArchetypeIndex::~ArchetypeIndex() noexcept
    {
        for (ArchetypeDataHeader* header : ice::array::slice(_archetype_data, 1))
        {
            ice::usize const size = ArchetypeDataHeader::calculate_meminfo(header->archetype_name, header->archetype_info);
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
        ice::ecs::detail::DataBlockFilter data_block_filter,
        ice::ecs::detail::DataBlockPool* data_block_pool
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
            data_block_pool = _default_block_pool.get();
        }

        ice::u32 const component_count = ice::count(archetype_info.component_identifiers);

        ICE_ASSERT(
            component_count >= 2,
            "You cannot register an archetype with no component types!"
        );

        ice::usize offset_name;
        ice::usize offset_ids;
        ice::usize offset_values;
        ice::meminfo const header_meminfo = ArchetypeDataHeader::calculate_meminfo(
            archetype_info.name,
            archetype_info,
            offset_name,
            offset_ids,
            offset_values
        );

        // We calculate the required space to store all information related to the current archetype.
        //  We need enough space for every components: identifier, size, alignment, offset
        ice::Memory const result = _allocator.allocate(header_meminfo);
        ArchetypeDataHeader* const data_header = reinterpret_cast<ArchetypeDataHeader*>(result.location);

        ice::Memory mem_archetype_name = ice::ptr_add(result, offset_name);
        ice::Memory mem_component_data = ice::ptr_add(result, offset_ids);
        ice::Memory mem_component_data_end = ice::ptr_add(result, header_meminfo.size);

        // Helper variables to access all component related values
        ice::StringID const* component_identifiers = nullptr;
        ice::u32 const* component_sizes = nullptr;
        ice::u32 const* component_alignments = nullptr;
        ice::u32* component_offsets = nullptr;

        {
            // Copy archetype name
            ice::memset(mem_archetype_name, 0);
            ice::memcpy(mem_archetype_name, archetype_info.name.data_view());

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

        data_header->archetype_name = ice::String{ (char const*) mem_archetype_name.location, archetype_info.name.size() };
        data_header->archetype_identifier = archetype_info.identifier;
        data_header->archetype_info.data_block_filter = data_block_filter;
        data_header->archetype_info.component_identifiers = ice::Span<ice::StringID const>{ component_identifiers, component_count };
        data_header->archetype_info.component_sizes = ice::Span<ice::u32 const>{ component_sizes, component_count };
        data_header->archetype_info.component_alignments = ice::Span<ice::u32 const>{ component_alignments, component_count };
        data_header->archetype_info.component_offsets = ice::Span<ice::u32 const>{ component_offsets, component_count };
        data_header->block_pool = data_block_pool;

        // We need now to calculate the number of entities that we can store in the remaining memory.
        //  Additionally calculate the offets each component array will be located at.
        {
            ice::u32 const component_entity_count_max = ice::ecs::detail::calculate_entity_count_for_space(
                data_header->archetype_info,
                data_block_pool->provided_block_size()
            );

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

                    next_component_offset += component_sizes[idx] * component_entity_count_max;
                }
            }
        }

        ice::u32 const archetype_index = ice::array::count(_archetype_data);
        data_header->archetype_info.archetype_instance = ice::ecs::detail::ArchetypeInstance{ archetype_index };

        ice::array::push_back(_archetype_data, data_header);
        ice::hashmap::set(_archetype_index, ice::hash(archetype_info.identifier), archetype_index);

        // Save the 'index' for the given name
        if (data_header->archetype_name.not_empty())
        {
            ice::hashmap::set(_archetype_names_index, ice::hash(data_header->archetype_name), archetype_index);
        }

        return archetype_info.identifier;
    }

    auto ArchetypeIndex::find_archetype_by_name(
        ice::String name
    ) const noexcept -> ice::ecs::Archetype
    {
        ice::u32 const instance_count = ice::array::count(_archetype_data);
        ice::u32 const instance_idx = ice::hashmap::get(_archetype_names_index, ice::hash(name), ice::u32_max);
        if (instance_idx >= instance_count)
        {
            return Archetype::Invalid;
        }

        return _archetype_data[instance_idx]->archetype_identifier;
    }

    void ArchetypeIndex::find_archetypes(
        ice::Array<ice::ecs::Archetype>& out_archetypes,
        ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
        ice::Span<ice::StringID const> query_tags
    ) const noexcept
    {
        ice::array::clear(out_archetypes);

        // We need to skip the first query entry if it's for `ice::ecs::Entity`
        // This is due to the fact that it's always there and is not taken into account when sorting components by identifiers.
        if (ice::span::front(query_info).identifier == ice::ecs::Constant_ComponentIdentifier<ice::ecs::Entity>)
        {
            query_info = ice::span::subspan(query_info, 1);
        }

        ice::u32 const required_tag_count = ice::count(query_tags);
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
            (required_component_count != 0) || (required_tag_count != 0),
            "An query without any required components or tags might impact performance as it will check every archetype possible for a match."
        );

        ice::u32 const total_required_type_count = required_component_count + required_tag_count;
        for (ArchetypeDataHeader const* entry : ice::array::slice(_archetype_data, 1))
        {
            ice::ecs::detail::ArchetypeInstanceInfo const& archetype_info = entry->archetype_info;

            ice::u32 const archetype_component_count = ice::count(archetype_info.component_identifiers);
            if (archetype_component_count < total_required_type_count)
            {
                continue;
            }

            bool const was_matched = ice::ecs::detail::contains_required_components(
                query_info,
                query_tags,
                archetype_info.component_identifiers
            );

            // If we don't match any component in a full optional query, we still skip this archetype.
            //  #todo: we should probably also check for the existance of the EntityHandle in the query. Then the check should be `> 1`
            if (was_matched)
            {
                ice::array::push_back(out_archetypes, entry->archetype_identifier);
            }
        }
    }

    void ArchetypeIndex::fetch_archetype_instance_infos(
        ice::Span<ice::ecs::Archetype const> archetypes,
        ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const*> out_instance_infos
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
        ice::Span<ice::ecs::detail::ArchetypeInstance const> archetype_instances,
        ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const*> out_instance_infos
    ) const noexcept
    {
        ICE_ASSERT(
            ice::count(archetype_instances) == ice::count(out_instance_infos),
            "Archetype instance fetch called with different input and output array sizes."
        );

        ice::u32 const instance_count = ice::array::count(_archetype_data);

        ice::u32 archetype_idx = 0;
        for (ice::ecs::detail::ArchetypeInstance archetype_instance : archetype_instances)
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

    void ArchetypeIndex::fetch_archetype_instance_info_by_index(
        ice::u32 index,
        ice::ecs::detail::ArchetypeInstanceInfo const*& out_instance_info
    ) const noexcept
    {
        ice::u32 const instance_count = ice::array::count(_archetype_data);
        ice::u32 const instance_idx = index;

        if (instance_idx < instance_count)
        {
            ArchetypeDataHeader const* const header = _archetype_data[instance_idx];

            out_instance_info = ice::addressof(header->archetype_info);
        }
        else
        {
            out_instance_info = nullptr;
        }
    }

    void ArchetypeIndex::fetch_archetype_instance_info_with_pool(
        ice::ecs::Archetype archetype,
        ice::ecs::detail::ArchetypeInstanceInfo const*& out_instance_info,
        ice::ecs::detail::DataBlockPool*& out_block_pool
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
        ice::ecs::detail::ArchetypeInstance archetype_instance,
        ice::ecs::detail::DataBlockPool*& out_block_pool
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
