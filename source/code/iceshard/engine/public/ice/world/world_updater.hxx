/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator_stack.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/engine_types.hxx>
#include <ice/asset_types.hxx>
#include <ice/stringid.hxx>
#include <ice/clock.hxx>

namespace ice
{

    struct EngineTaskContainer;

    struct WorldStateParams
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::Engine& engine;
        ice::EngineSchedulers thread;
    };

    struct WorldUpdater
    {
        virtual ~WorldUpdater() noexcept = default;

        virtual void update(
            ice::TaskContainer& out_tasks,
            ice::Span<ice::Shard const> event_shards
        ) noexcept = 0;
    };

} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::WorldStateParams const*> = ice::shard_payloadid("ice::WorldStateParams const*");
