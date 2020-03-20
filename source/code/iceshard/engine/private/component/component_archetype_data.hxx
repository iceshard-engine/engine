#pragma once
#include <core/pod/array.hxx>
#include <core/cexpr/stringid.hxx>
#include <iceshard/component/component_block.hxx>
#include <iceshard/component/component_block_allocator.hxx>

namespace iceshard::ecs
{

    struct ArchetypeData
    {
        ArchetypeData(
            core::allocator& alloc,
            iceshard::ComponentBlockAllocator* block_alloc
        ) noexcept;

        ~ArchetypeData() noexcept = default;

        core::stringid_type identifier;
        core::pod::Array<core::stringid_type> components;
        core::pod::Array<uint32_t> sizes;
        core::pod::Array<uint32_t> alignments;
        core::pod::Array<uint32_t> offsets;

        iceshard::ComponentBlockAllocator* block_allocator;
        iceshard::ComponentBlock* block = nullptr;

        uint32_t block_count = 0;
    };

    auto archetype_identifier(
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type;

    auto archetype_identifier_extended(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type;

    auto archetype_identifier_reduced(
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> core::stringid_type;

    auto create_archetype(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t> const& sizes,
        core::pod::Array<uint32_t> const& alignments
    ) noexcept -> ArchetypeData*;

    auto create_archetype_extended(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components,
        core::pod::Array<uint32_t> const& sizes,
        core::pod::Array<uint32_t> const& alignments
    ) noexcept -> ArchetypeData*;

    auto create_archetype_reduced(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc,
        iceshard::ecs::ArchetypeData const* base_archetype,
        core::pod::Array<core::stringid_type> const& components
    ) noexcept -> ArchetypeData*;

} // namespace iceshard::ecs
