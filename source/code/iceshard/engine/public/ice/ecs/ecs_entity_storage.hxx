/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice::ecs
{

    class EntityStorage : public ice::ecs::QueryProvider
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ecs::ArchetypeIndex const& archetype_index
        ) noexcept;

        ~EntityStorage() noexcept;

        auto entities() noexcept -> ice::ecs::EntityIndex&;
        auto entities() const noexcept -> ice::ecs::EntityIndex const&;

        void update_archetypes() noexcept;

        void execute_operations(
            ice::ecs::EntityOperations const& operations,
            ice::ShardContainer& out_shards
        ) noexcept;

        auto find_archetype(
            ice::String name
        ) const noexcept -> ice::ecs::Archetype override;

        auto query_data_slot(
            ice::ecs::Entity entity
        ) const noexcept -> ice::ecs::EntityDataSlot override;

        auto query_data_slots(
            ice::Span<ice::ecs::Entity const> requested,
            ice::Span<ice::ecs::EntityDataSlot> out_data_slots
        ) const noexcept -> ice::ucount override;

    protected:
        void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::StringID const> query_tags,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::detail::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::ecs::EntityIndex _entity_index;
        ice::ecs::ArchetypeIndex const& _archetype_index;

        ice::HashMap<ice::ecs::QueryAccessTracker*> _access_trackers;
        ice::Array<ice::ecs::DataBlock> _head_blocks;
        ice::Array<ice::ecs::DataBlock*> _data_blocks;
        ice::Array<ice::ecs::EntityDataSlot> _data_slots;
    };

} // namespace ice::ecs
