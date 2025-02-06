/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_assembly.hxx>
#include <ice/world/world_trait_types.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    struct WorldUpdater
    {
        virtual ~WorldUpdater() noexcept = default;

        virtual void pre_update(
            ice::ShardContainer& out_shards
        ) noexcept = 0;

        virtual void update(
            ice::TaskContainer& out_tasks,
            ice::TraitParams const& trait_params,
            ice::Span<ice::Shard const> event_shards
        ) noexcept = 0;

        virtual void update(
            ice::StringID_Arg world_name,
            ice::TaskContainer& out_tasks,
            ice::TraitParams const& trait_params,
            ice::Span<ice::Shard const> event_shards
        ) noexcept = 0;
    };

} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::WorldStateParams const*> = ice::shard_payloadid("ice::WorldStateParams const*");
