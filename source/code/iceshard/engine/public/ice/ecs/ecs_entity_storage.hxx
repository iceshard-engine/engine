/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice::ecs
{

    class ArchetypeIndex;

    class EntityOperations;

    class EntityStorage : public ice::ecs::QueryProvider
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ecs::ArchetypeIndex const& archetype_index
        ) noexcept;

        ~EntityStorage() noexcept;

        void update_archetypes() noexcept;

        void execute_operations(
            ice::ecs::EntityOperations const& operations,
            ice::ShardContainer& out_shards
        ) noexcept;

        auto find_archetype(
            ice::String name
        ) const noexcept -> ice::ecs::Archetype override;

        auto resolve_entities(
            ice::Span<ice::ecs::Entity const> requested,
            ice::Span<ice::ecs::EntityHandle> resolved
        ) const noexcept -> ice::ucount override;

    protected:
        void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::ecs::ArchetypeIndex const& _archetype_index;

        ice::HashMap<ice::ecs::EntityHandle> _entities;
        ice::HashMap<ice::ecs::QueryAccessTracker*> _access_trackers;
        ice::Array<ice::ecs::DataBlock> _head_blocks;
        ice::Array<ice::ecs::DataBlock*> _data_blocks;
    };

} // namespace ice::ecs
