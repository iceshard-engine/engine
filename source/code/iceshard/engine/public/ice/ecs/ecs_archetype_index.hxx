#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block_pool.hxx>
#include <ice/ecs/ecs_query_details.hxx>
#include <ice/container_types.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    class DataBlockPool;

    class ArchetypeIndex
    {
    public:
        ArchetypeIndex(
            ice::Allocator& alloc
        ) noexcept;

        ~ArchetypeIndex() noexcept;

        auto registered_archetype_count() const noexcept -> ice::u32;

        auto register_archetype(
            ice::ecs::ArchetypeInfo const& archetype_info,
            ice::ecs::DataBlockPool* data_block_pool = nullptr
        ) noexcept -> ice::ecs::Archetype;

        void find_archetypes(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Array<ice::ecs::Archetype>& out_archetypes
        ) const noexcept;

        void fetch_archetype_instance_infos(
            ice::Span<ice::ecs::Archetype const> archetypes,
            ice::Span<ice::ecs::ArchetypeInstanceInfo const*> out_instance_infos
        ) const noexcept;

        void fetch_archetype_instance_infos(
            ice::Span<ice::ecs::ArchetypeInstance const> archetype_instances,
            ice::Span<ice::ecs::ArchetypeInstanceInfo const*> out_instance_infos
        ) const noexcept;

        void fetch_archetype_instance_info_with_pool(
            ice::ecs::Archetype archetype,
            ice::ecs::ArchetypeInstanceInfo const*& out_instance_info,
            ice::ecs::DataBlockPool*& out_block_pool
        ) const noexcept;

        void fetch_archetype_instance_pool(
            ice::ecs::ArchetypeInstance archetype,
            ice::ecs::DataBlockPool*& out_block_pool
        ) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ecs::DataBlockPool _default_block_pool;

        ice::HashMap<ice::u32> _archetype_index;

        struct ArchetypeDataHeader;
        ice::Array<ArchetypeDataHeader*> _archetype_data;
    };

} // namespace ice::ecs
