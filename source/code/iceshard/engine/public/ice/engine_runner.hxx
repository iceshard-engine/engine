/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/input/input_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/platform_event.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/shard.hxx>
#include <ice/stringid.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_types.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    using EngineFrameFactoryUserdata = void*;
    using EngineFrameFactory = auto(*)(
        ice::Allocator&, ice::EngineFrameData&, EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>;

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

        ice::u32 concurrent_frame_count = 2;
        ice::EngineFrameFactory frame_factory;
        ice::EngineFrameFactoryUserdata frame_factory_userdata;
        ice::EngineSchedulers schedulers;
    };

    struct EngineRunner
    {
        virtual ~EngineRunner() noexcept = default;

        virtual auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> = 0;
        virtual auto update_frame(ice::EngineFrame& frame, ice::EngineFrame const& prev_frame, ice::Clock const& clock) noexcept -> ice::Task<> = 0;
        virtual void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept = 0;
    };

    static constexpr ice::ShardID ShardID_FrameUpdate = "event/engine/frame-update`ice::EngineFrameUpdate const*"_shardid;

} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::EngineFrameUpdate const*> = ice::shard_payloadid("ice::EngineFrameUpdate const*");
