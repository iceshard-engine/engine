/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>
#include <ice/asset_storage.hxx>
#include <ice/task_stage.hxx>
#include <ice/clock.hxx>

namespace ice::gfx
{

    struct GfxStateChange
    {
        ice::AssetStorage& assets;
        ice::gfx::GfxContext& context;
        ice::gfx::GfxStageRegistry& stages;
    };

    struct GfxFrameUpdate
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::EngineFrame const& frame;
        ice::gfx::GfxContext& context;
    };

    static constexpr ice::ShardID ShardID_GfxStartup = "event/gfx/startup`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxResume = "event/gfx/resume`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSwapchainReset = "event/gfx/swapchain-reset`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSuspend = "event/gfx/suspend`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxShutdown = "event/gfx/shutdown`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxFrameUpdate = "event/gfx/frame-update`ice::gfx::GfxFrameUpdate const*"_shardid;

} // namespace ice::gfx

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxFrameUpdate const*> = ice::shard_payloadid("ice::gfx::GfxFrameUpdate const*");

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxStateChange const*> = ice::shard_payloadid("ice::gfx::GfxStateChange const*");
