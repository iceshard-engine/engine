#pragma once
#include <ice/pod/array.hxx>
#include <ice/pod/hash.hxx>
#include <ice/archetype/archetype_index.hxx>

namespace ice
{

    struct IceArchetypeInstance
    {
        ice::ArchetypeBlockAllocator* block_allocator;
        ice::u32 block_max_entity_count;
        ice::u32 block_base_alignment;
        ice::u32 instance_offset;
        ice::u32 component_count;
    };

    class IceArchetypeIndex final : public ArchetypeIndex
    {
    public:
        IceArchetypeIndex(
            ice::Allocator& alloc
        ) noexcept;
        ~IceArchetypeIndex() noexcept override = default;

        auto register_archetype(
            ice::ArchetypeBlockAllocator* block_allocator,
            ice::Span<ice::ArchetypeComponent const> archetype_components
        ) noexcept -> ice::ArchetypeHandle override;

        auto find_archetype(
            ice::ArchetypeQueryCriteria query_criteria
        ) const noexcept -> ice::ArchetypeHandle override;

        bool find_archetypes(
            ice::ArchetypeQueryCriteria query_criteria,
            ice::pod::Array<ice::ArchetypeHandle>& archetypes_out
        ) const noexcept override;

        bool archetype_info(
            ice::Span<ice::ArchetypeHandle const> archetypes,
            ice::Span<ice::ArchetypeInfo> archetype_infos_out
        ) const noexcept override;

        auto archetype_info(
            ice::IceArchetypeInstance instance
        ) const noexcept -> ice::ArchetypeInfo;

    private:
        ice::pod::Hash<ice::IceArchetypeInstance> _archetypes;
        ice::pod::Array<ice::StringID> _archetype_component_names;
        ice::pod::Array<ice::u32> _archetype_component_sizes;
        ice::pod::Array<ice::u32> _archetype_component_alignments;
        ice::pod::Array<ice::u32> _archetype_component_offsets;
    };

} // namespace ice
