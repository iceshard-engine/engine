#pragma once
#include <ice/stringid.hxx>
#include <ice/span.hxx>

namespace ice
{

    class ArchetypeBlockAllocator;

    struct ArchetypeQueryCriteria
    {
        ice::Span<ice::StringID const> components;
        ice::Span<bool const> optional;
    };

    using ArchetypeIndexQuery = ArchetypeQueryCriteria;

    struct ArchetypeInfo
    {
        ice::ArchetypeBlockAllocator* block_allocator;
        ice::u32 block_base_alignment;
        ice::u32 block_max_entity_count;
        ice::Span<ice::StringID const> components;
        ice::Span<ice::u32 const> sizes;
        ice::Span<ice::u32 const> alignments;
        ice::Span<ice::u32 const> offsets;
    };

} // namespace ice
