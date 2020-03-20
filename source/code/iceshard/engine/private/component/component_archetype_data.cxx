#include "component_archetype_data.hxx"

#include <core/pod/algorithm.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_block_operation.hxx>

namespace iceshard::ecs
{
    namespace detail
    {

        //! \brief Mixes the two hash values and returns a new one
        auto hash_mix(uint64_t left, uint64_t right) noexcept -> uint64_t
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

        auto sort_component_predicate(
            core::stringid_arg_type left,
            core::stringid_arg_type right
        ) noexcept
        {
            return core::hash(left) < core::hash(right);
        }

    } // namespace detail

    auto archetype_identifier(
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type
    {
        uint64_t hashed_id = core::hash(core::stringid_invalid);
        for (core::stringid_arg_type component : components)
        {
            hashed_id = detail::hash_mix(hashed_id, core::hash(component));
        }
        return { core::stringid_hash_type{ hashed_id } };
    }

    auto archetype_identifier_extended(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type
    {
        uint64_t hashed_id = core::hash(base_archetype->identifier);
        for (core::stringid_arg_type component : components)
        {
            hashed_id = detail::hash_mix(hashed_id, core::hash(component));
        }
        return { core::stringid_hash_type{ hashed_id } };
    }

    auto archetype_identifier_reduced(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type
    {
        uint64_t hashed_id = core::hash(core::stringid_invalid);
        for (core::stringid_arg_type component : base_archetype->components)
        {
            bool contains = false;
            for (auto excluded_component : components)
            {
                contains |= excluded_component == component;
            }

            if (contains == false)
            {
                hashed_id = detail::hash_mix(hashed_id, core::hash(component));
            }
        }
        return { core::stringid_hash_type{ hashed_id } };
    }

    auto create_archetype(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t> const& sizes,
        core::pod::Array<uint32_t> const& alignments
    ) noexcept -> ArchetypeData*
    {
        ArchetypeData* archetype_data = alloc.make<ArchetypeData>(alloc, block_alloc);
        archetype_data->components = components;
        archetype_data->sizes = sizes;
        archetype_data->alignments = alignments;

        core::pod::array::resize(archetype_data->offsets, core::pod::array::size(archetype_data->components));

        // Sort the components before creating an identifier
        archetype_data->identifier = archetype_identifier(archetype_data->components);

        archetype_data->block = archetype_data->block_allocator->alloc_block();
        archetype_data->block_count = 1;

        iceshard::detail::component_block_prepare(
            archetype_data->block,
            core::pod::array::begin(archetype_data->sizes),
            core::pod::array::begin(archetype_data->alignments),
            core::pod::array::size(archetype_data->components),
            core::pod::array::begin(archetype_data->offsets)
        );

        return archetype_data;
    }

    auto create_archetype_extended(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t> const& sizes,
        core::pod::Array<uint32_t> const& alignments
    ) noexcept -> ArchetypeData*
    {
        ArchetypeData* archetype_data = alloc.make<ArchetypeData>(alloc, block_alloc);
        archetype_data->components = base_archetype->components;
        archetype_data->sizes = base_archetype->sizes;
        archetype_data->alignments = base_archetype->alignments;

        core::pod::array::push_back(archetype_data->components, components);
        core::pod::array::push_back(archetype_data->sizes, sizes);
        core::pod::array::push_back(archetype_data->alignments, alignments);
        core::pod::array::resize(archetype_data->offsets, core::pod::array::size(archetype_data->components));

        // Sort the components before creating an identifier
        archetype_data->identifier = archetype_identifier(archetype_data->components);

        archetype_data->block = archetype_data->block_allocator->alloc_block();
        archetype_data->block_count = 1;

        iceshard::detail::component_block_prepare(
            archetype_data->block,
            core::pod::array::begin(archetype_data->sizes),
            core::pod::array::begin(archetype_data->alignments),
            core::pod::array::size(archetype_data->components),
            core::pod::array::begin(archetype_data->offsets)
        );

        return archetype_data;
    }

    auto create_archetype_reduced(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> ArchetypeData*
    {
        ArchetypeData* archetype_data = alloc.make<ArchetypeData>(alloc, block_alloc);

        uint32_t const excluded_component_count = core::pod::array::size(components);
        uint32_t const base_component_count = core::pod::array::size(base_archetype->components);
        uint32_t const final_component_count = base_component_count - excluded_component_count;

        core::pod::array::resize(archetype_data->components, final_component_count);
        core::pod::array::resize(archetype_data->sizes, final_component_count);
        core::pod::array::resize(archetype_data->alignments, final_component_count);
        core::pod::array::resize(archetype_data->offsets, core::pod::array::size(archetype_data->components));

        uint32_t dst_idx = 0;

        for (uint32_t idx = 0; idx < base_component_count; ++idx)
        {
            bool contains = false;
            for (core::stringid_arg_type excluded : components)
            {
                contains |= excluded == base_archetype->components[idx];
            }

            if (contains == false)
            {
                archetype_data->components[dst_idx] = base_archetype->components[idx];
                archetype_data->sizes[dst_idx] = base_archetype->sizes[idx];
                archetype_data->alignments[dst_idx] = base_archetype->alignments[idx];
                dst_idx += 1;
            }
        }

        // Sort the components before creating an identifier
        archetype_data->identifier = archetype_identifier(archetype_data->components);

        archetype_data->block = archetype_data->block_allocator->alloc_block();
        archetype_data->block_count = 1;

        iceshard::detail::component_block_prepare(
            archetype_data->block,
            core::pod::array::begin(archetype_data->sizes),
            core::pod::array::begin(archetype_data->alignments),
            core::pod::array::size(archetype_data->components),
            core::pod::array::begin(archetype_data->offsets)
        );

        return archetype_data;
    }

    ArchetypeData::ArchetypeData(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc
    ) noexcept
        : identifier{ core::stringid_invalid }
        , components{ alloc }
        , sizes{ alloc }
        , alignments{ alloc }
        , offsets{ alloc }
        , block_allocator{ block_alloc }
    {
    }

} // namespace iceshard::ecs
