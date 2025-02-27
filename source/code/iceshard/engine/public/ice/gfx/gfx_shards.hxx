/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
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
        ice::gfx::GfxFrameStages& stages;
        ice::gfx::GfxStageRegistry& registry;
    };

    struct GfxFrameUpdate
    {
        ice::EngineFrame& frame;
        ice::gfx::GfxContext& context;
    };

    struct RenderFrameUpdate
    {
        ice::EngineFrame const& frame;
        ice::gfx::GfxContext& context;
        ice::gfx::GfxFrameStages& stages;
    };

    static constexpr ice::ShardID ShardID_GfxStartup = "event/gfx/startup`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxResume = "event/gfx/resume`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSwapchainReset = "event/gfx/swapchain-reset`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSuspend = "event/gfx/suspend`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxShutdown = "event/gfx/shutdown`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxFrameUpdate = "event/gfx/frame-update`ice::gfx::GfxFrameUpdate const*"_shardid;

    static constexpr ice::ShardID ShardID_RenderFrameUpdate = "event/render/frame-update`ice::gfx::RenderFrameUpdate const*"_shardid;

} // namespace ice::gfx

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxFrameUpdate const*> = ice::shard_payloadid("ice::gfx::GfxFrameUpdate const*");
template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::RenderFrameUpdate const*> = ice::shard_payloadid("ice::gfx::RenderFrameUpdate const*");

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxStateChange const*> = ice::shard_payloadid("ice::gfx::GfxStateChange const*");
