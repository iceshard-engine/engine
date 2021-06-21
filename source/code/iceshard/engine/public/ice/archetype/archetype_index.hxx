#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/pod/collections.hxx>
#include <ice/entity/entity_archetype.hxx>
#include <ice/archetype/archetype_info.hxx>

namespace ice
{

    class ArchetypeBlockAllocator;

    struct ArchetypeIndexOptions
    {
        bool allow_jit_archetype_registration = true;
        bool allow_archetype_duplicate_registration = false;
    };

    class ArchetypeIndex
    {
    public:
        virtual ~ArchetypeIndex() noexcept = default;

        virtual auto register_archetype(
            ice::ArchetypeBlockAllocator* block_allocator,
            ice::Span<ice::ArchetypeComponent const> archetype_components
        ) noexcept -> ice::ArchetypeHandle = 0;

        template<typename... Components>
        auto register_archetype(
            ice::ArchetypeBlockAllocator* block_allocator,
            ice::Archetype<Components...> archetype
        ) noexcept -> ice::ArchetypeHandle;

        template<typename... Components>
        auto register_archetype(
            ice::ArchetypeBlockAllocator* block_allocator
        ) noexcept -> ice::ArchetypeHandle;

        virtual auto find_archetype(
            ice::ArchetypeQueryCriteria query_criteria
        ) const noexcept -> ice::ArchetypeHandle = 0;

        virtual bool find_archetypes(
            ice::ArchetypeQueryCriteria query_criteria,
            ice::pod::Array<ice::ArchetypeHandle>& archetypes_out
        ) const noexcept = 0;

        virtual bool archetype_info(
            ice::Span<ice::ArchetypeHandle const> archetypes,
            ice::pod::Array<ice::ArchetypeInfo>& archetype_infos_out
        ) const noexcept = 0;
    };

    auto create_archetype_index(
        ice::Allocator& alloc,
        ice::ArchetypeIndexOptions options = { }
    ) noexcept -> ice::UniquePtr<ice::ArchetypeIndex>;

    template<typename... Components>
    auto ArchetypeIndex::register_archetype(
        ice::ArchetypeBlockAllocator* block_allocator,
        ice::Archetype<Components...> archetype
    ) noexcept -> ice::ArchetypeHandle
    {
        return register_archetype(block_allocator, archetype.components);
    }

    template<typename... Components>
    auto ArchetypeIndex::register_archetype(
        ice::ArchetypeBlockAllocator* block_allocator
    ) noexcept -> ice::ArchetypeHandle
    {
        return register_archetype(block_allocator, Archetype<Components...>{ });
    }

} // namespace ice
