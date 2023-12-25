/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/engine_types.hxx>
#include <ice/asset_types.hxx>
#include <ice/stringid.hxx>
#include <ice/clock.hxx>

namespace ice
{

    struct EngineWorldUpdate
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
            ice::EngineFrame& frame,
            ice::EngineWorldUpdate const& world_update,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept = 0;

        virtual void force_update(
            ice::StringID_Arg world_name,
            ice::Shard shard,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept = 0;

        virtual void update(
            ice::Shard shard,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept = 0;

        virtual void update(
            ice::ShardContainer const& shards,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept = 0;
    };

} // namespace ice
