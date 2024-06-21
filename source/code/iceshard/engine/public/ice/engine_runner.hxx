/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/input/input_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/platform_event.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/shard_container.hxx>
#include <ice/stringid.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_types.hxx>
#include <ice/task_utils.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    using EngineFrameFactoryUserdata = void*;

    using EngineFrameFactory = auto(*)(
        ice::Allocator&, ice::EngineFrameData&, EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>;

    struct EngineTaskContainer
    {
        virtual ~EngineTaskContainer() noexcept = default;

        virtual void execute(ice::Task<> task) noexcept = 0;
    };

    struct EngineFrameUpdate
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::EngineFrame& frame;
        ice::EngineFrame const& last_frame;
        ice::EngineSchedulers thread;
    };

    struct EngineRunnerCreateInfo
    {
        ice::Engine& engine;
        ice::Clock const& clock;
        ice::u32 concurrent_frame_count = 2;
        ice::EngineFrameFactory frame_factory;
        ice::EngineFrameFactoryUserdata frame_factory_userdata;
        ice::EngineSchedulers schedulers;
    };

    struct EngineRunner
    {
        virtual ~EngineRunner() noexcept = default;

        virtual auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> = 0;
        virtual auto update_frame(ice::EngineFrame& current_frame, ice::EngineFrame const& last_frame) noexcept -> ice::Task<> = 0;
        virtual void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept = 0;

        virtual auto apply_entity_operations(ice::ShardContainer& out_shards) noexcept -> ice::Task<> = 0;
    };

    static constexpr ice::ShardID ShardID_FrameUpdate = "event/engine/frame-update`ice::EngineFrameUpdate const*"_shardid;

} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::EngineFrameUpdate const*> = ice::shard_payloadid("ice::EngineFrameUpdate const*");
