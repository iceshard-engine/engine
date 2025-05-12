/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block_pool.hxx>
#include <ice/ecs/ecs_query_details.hxx>
#include <ice/container_types.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    //! \brief Stores and provides access to all archetypes that are registered by the engine, modules and game.
    //!
    //! \warning The current implementation does not allow to remove any registered `Archetype`, so unloading modules that hold type
    //!   definitions is not supported.
    class ArchetypeIndex
    {
    public:
        ArchetypeIndex(ice::Allocator& alloc) noexcept;
        ~ArchetypeIndex() noexcept;

        auto registered_archetype_count() const noexcept -> ice::u32;

        template<ice::ecs::Component... Components>
        auto new_archetype(
            ice::String name = {},
            ice::ecs::detail::DataBlockPool * data_block_pool = nullptr
        ) noexcept -> ice::ecs::Archetype;

        auto register_archetype(
            ice::ecs::ArchetypeInfo const& archetype_info,
            ice::ecs::detail::DataBlockPool* data_block_pool = nullptr
        ) noexcept -> ice::ecs::Archetype;

        auto find_archetype_by_name(
            ice::String name
        ) const noexcept -> ice::ecs::Archetype;

        void find_archetypes(
            ice::Array<ice::ecs::Archetype>& out_archetypes,
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::StringID const> query_tags = {}
        ) const noexcept;

        void fetch_archetype_instance_infos(
            ice::Span<ice::ecs::Archetype const> archetypes,
            ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const*> out_instance_infos
        ) const noexcept;

        void fetch_archetype_instance_infos(
            ice::Span<ice::ecs::detail::ArchetypeInstance const> archetype_instances,
            ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const*> out_instance_infos
        ) const noexcept;

        void fetch_archetype_instance_info_by_index(
            ice::u32 index,
            ice::ecs::detail::ArchetypeInstanceInfo const*& out_instance_info
        ) const noexcept;

        void fetch_archetype_instance_info_with_pool(
            ice::ecs::Archetype archetype,
            ice::ecs::detail::ArchetypeInstanceInfo const*& out_instance_info,
            ice::ecs::detail::DataBlockPool*& out_block_pool
        ) const noexcept;

        void fetch_archetype_instance_pool(
            ice::ecs::detail::ArchetypeInstance archetype,
            ice::ecs::detail::DataBlockPool*& out_block_pool
        ) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<ice::ecs::detail::DataBlockPool> _default_block_pool;

        ice::HashMap<ice::u32> _archetype_index;
        ice::HashMap<ice::u32> _archetype_names_index;

        struct ArchetypeDataHeader;
        ice::Array<ArchetypeDataHeader*> _archetype_data;
    };

    template<ice::ecs::Component... Components>
    inline auto ArchetypeIndex::new_archetype(
        ice::String name,
        ice::ecs::detail::DataBlockPool* data_block_pool
    ) noexcept -> ice::ecs::Archetype
    {
        ice::ecs::ArchetypeInfo info = ice::ecs::Constant_ArchetypeDefinition<Components...>;
        info.name = name;
        return this->register_archetype(info, data_block_pool);
    }

} // namespace ice::ecs
