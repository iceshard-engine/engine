#include "ice_archetype_index.hxx"
#include <ice/assert.hxx>
#include <ice/log.hxx>

#include <ice/entity/entity_archetype.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>

namespace ice
{

    constexpr auto align_forward_offset(
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

    IceArchetypeIndex::IceArchetypeIndex(
        ice::Allocator& alloc
    ) noexcept
        : _archetypes{ alloc }
        , _archetype_component_names{ alloc }
        , _archetype_component_sizes{ alloc }
        , _archetype_component_alignments{ alloc }
        , _archetype_component_offsets{ alloc }
    {
        static constexpr ice::u32 const preallocated_archetype_count = 100;
        static constexpr ice::u32 const average_archetype_component_count = 10;

        ice::pod::hash::reserve(
            _archetypes,
            static_cast<ice::u32>(preallocated_archetype_count * 1.4)
        );
        ice::pod::array::reserve(
            _archetype_component_names,
            preallocated_archetype_count * average_archetype_component_count
        );
        ice::pod::array::reserve(
            _archetype_component_sizes,
            preallocated_archetype_count * average_archetype_component_count
        );
        ice::pod::array::reserve(
            _archetype_component_alignments,
            preallocated_archetype_count * average_archetype_component_count
        );
        ice::pod::array::reserve(
            _archetype_component_offsets,
            preallocated_archetype_count * average_archetype_component_count
        );
    }

    auto IceArchetypeIndex::register_archetype(
        ice::ArchetypeBlockAllocator* block_allocator,
        ice::Span<ice::ArchetypeComponent const> archetype_components
    ) noexcept -> ice::ArchetypeHandle
    {
        ice::ArchetypeHandle const handle = archetype_handle(
            archetype_components
        );

        if (ice::pod::hash::has(_archetypes, ice::hash(handle)))
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "This archetype is already registered!"
            );
            return handle;
        }

        IceArchetypeInstance archetype_instance{ };
        archetype_instance.block_allocator = block_allocator;
        archetype_instance.block_max_entity_count = 0;
        archetype_instance.component_count = ice::size(archetype_components);
        archetype_instance.instance_offset = ice::pod::array::size(_archetype_component_names);

        // TODO check if components are sorted.
        ice::u32 max_alignment = 0;
        for (ice::ArchetypeComponent const& component : archetype_components)
        {
            max_alignment = ice::max(max_alignment, component.alignment);
            ice::pod::array::push_back(_archetype_component_names, component.name);
            ice::pod::array::push_back(_archetype_component_sizes, component.size);
            ice::pod::array::push_back(_archetype_component_alignments, component.alignment);
        }

        ice::u32 const component_size_sum = std::accumulate(
            ice::pod::begin(_archetype_component_sizes) + archetype_instance.instance_offset,
            ice::pod::end(_archetype_component_sizes),
            0
        );
        ice::u32 const component_alignment_sum = std::accumulate(
            ice::pod::begin(_archetype_component_alignments) + archetype_instance.instance_offset,
            ice::pod::end(_archetype_component_alignments),
            0
        );

        ice::u32 const block_size = block_allocator->block_size();
        ice::u32 const available_block_size = block_size - (component_alignment_sum + max_alignment);
        archetype_instance.block_base_alignment = max_alignment;
        archetype_instance.block_max_entity_count = available_block_size / component_size_sum;

        ice::u32 next_component_offset = 0;
        for (ice::ArchetypeComponent const& component : archetype_components)
        {
            next_component_offset = align_forward_offset(next_component_offset, component.alignment);

            ice::pod::array::push_back(
                _archetype_component_offsets,
                next_component_offset
            );

            next_component_offset += component.size * archetype_instance.block_max_entity_count;
        }

        ice::pod::hash::set(
            _archetypes,
            ice::hash(handle),
            archetype_instance
        );
        return handle;
    }

    auto IceArchetypeIndex::find_archetype(
        ice::ArchetypeQueryCriteria query_criteria
    ) const noexcept -> ice::ArchetypeHandle
    {
        bool query_has_optional_components = false;
        for (bool const optional : query_criteria.optional)
        {
            query_has_optional_components |= optional;
        }

        if (query_has_optional_components)
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "The query contains coponents flagged as optional. These flags will be ignored!"
            );
        }

        ice::u32 const query_component_count = static_cast<ice::u32>(query_criteria.components.size());
        ice::ArchetypeHandle result_handle = ArchetypeHandle::Invalid;

        for (auto const& entry : _archetypes)
        {
            // We only want to find the exact archetype
            if (entry.value.component_count == query_component_count)
            {
                continue;
            }

            ArchetypeInfo const candidate_archetype = archetype_info(entry.value);

            bool contains_required_components = true;
            for (ice::u32 idx = 0; idx < query_component_count && contains_required_components; ++idx)
            {
                bool contains_component = false;
                for (ice::StringID_Arg candidate_component : candidate_archetype.components)
                {
                    contains_component |= candidate_component == query_criteria.components[idx];
                }

                contains_required_components &= contains_component;
            }

            if (contains_required_components)
            {
                result_handle = static_cast<ice::ArchetypeHandle>(entry.key);
                break;
            }
        }

        return result_handle;
    }

    bool IceArchetypeIndex::find_archetypes(
        ice::ArchetypeQueryCriteria query_criteria,
        ice::pod::Array<ice::ArchetypeHandle>& archetypes_out
    ) const noexcept
    {
        ICE_ASSERT(
            query_criteria.components.size() == query_criteria.optional.size(),
            "The member span 'optional' needs to have same size as `components` span! [ {} != {} ]",
            query_criteria.components.size(), query_criteria.optional.size()
        );

        ice::pod::array::clear(archetypes_out);

        ice::u32 const query_component_count = static_cast<ice::u32>(query_criteria.components.size());

        for (auto const& entry : _archetypes)
        {
            ArchetypeInfo const candidate_archetype = archetype_info(entry.value);

            bool contains_required_components = true;
            for (ice::u32 idx = 0; idx < query_component_count && contains_required_components; ++idx)
            {
                if (query_criteria.optional[idx] == false)
                {
                    bool contains_component = false;
                    for (ice::StringID_Arg candidate_component : candidate_archetype.components)
                    {
                        contains_component |= candidate_component == query_criteria.components[idx];
                    }

                    contains_required_components &= contains_component;
                }
            }

            if (contains_required_components)
            {
                ice::pod::array::push_back(
                    archetypes_out,
                    static_cast<ice::ArchetypeHandle>(entry.key)
                );
            }
        }

        return ice::pod::array::empty(archetypes_out) == false;
    }

    bool IceArchetypeIndex::archetype_info(
        ice::Span<ice::ArchetypeHandle const> archetypes,
        ice::Span<ice::ArchetypeInfo> archetype_infos_out
    ) const noexcept
    {
        static ice::IceArchetypeInstance null_instance{ .component_count = 0 };

        ice::u32 archetype_idx = 0;
        ice::u32 archetypes_found = 0;
        for (ice::ArchetypeHandle const handle : archetypes)
        {
            ice::IceArchetypeInstance const& archetype_instance = ice::pod::hash::get(
                _archetypes,
                ice::hash(handle),
                null_instance
            );

            //ICE_ASSERT(
            //    archetype_instance.component_count > 0,
            //    "Invalid archetype handle: {:X}",
            //    ice::hash(handle)
            //);

            if (archetype_instance.component_count > 0)
            {
                archetypes_found += 1;
                archetype_infos_out[archetype_idx] = archetype_info(archetype_instance);
            }

            archetype_idx += 1;
        }

        return archetypes_found == archetype_idx;
    }

    auto IceArchetypeIndex::archetype_info(
        ice::IceArchetypeInstance instance
    ) const noexcept -> ice::ArchetypeInfo
    {
        ice::StringID const* const component_names = std::addressof(
            _archetype_component_names[instance.instance_offset]
        );
        ice::u32 const* const component_sizes = std::addressof(
            _archetype_component_sizes[instance.instance_offset]
        );
        ice::u32 const* const component_alignments = std::addressof(
            _archetype_component_alignments[instance.instance_offset]
        );
        ice::u32 const* const component_offsets = std::addressof(
            _archetype_component_offsets[instance.instance_offset]
        );

        return ice::ArchetypeInfo{
            .block_allocator = instance.block_allocator,
            .block_base_alignment = instance.block_base_alignment,
            .block_max_entity_count = instance.block_max_entity_count,
            .components = ice::Span<ice::StringID const>{ component_names, component_names + instance.component_count },
            .sizes = ice::Span<ice::u32 const>{ component_sizes, component_sizes + instance.component_count },
            .alignments = ice::Span<ice::u32 const>{ component_alignments, component_alignments + instance.component_count },
            .offsets = ice::Span<ice::u32 const>{ component_offsets, component_offsets + instance.component_count },
        };
    }

    auto create_archetype_index(
        ice::Allocator& alloc,
        ice::ArchetypeIndexOptions options
    ) noexcept -> ice::UniquePtr<ice::ArchetypeIndex>
    {
        return ice::make_unique<ice::ArchetypeIndex, ice::IceArchetypeIndex>(alloc, alloc);
    }

} // namespace ice
