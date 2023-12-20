/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
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
        ice::gfx::GfxDevice& device;
        ice::render::Renderpass renderpass;
        ice::TaskStage<ice::render::CommandBuffer> frame_transfer;
        ice::TaskStage<> frame_end;
    };

    struct GfxFrameUpdate
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::EngineFrame const& frame;
        ice::gfx::GfxDevice& device;
        ice::gfx::GfxStageRegistry& stages;
    };

    static constexpr ice::ShardID ShardID_GfxStartup = "event/gfx/startup`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxResume = "event/gfx/resume`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSwapchainReset = "event/gfx/swapchain-reset`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxSuspend = "event/gfx/suspend`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxShutdown = "event/gfx/shutdown`ice::gfx::GfxStateChange const*"_shardid;
    static constexpr ice::ShardID ShardID_GfxFrameUpdate = "event/gfx/frame-update`ice::gfx::GfxFrameUpdate const*"_shardid;

} // namespace ice::gfx

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxFrameUpdate const*> = ice::shard_payloadid("ice::gfx::GfxFrameUpdate const*");

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::GfxStateChange const*> = ice::shard_payloadid("ice::gfx::GfxStateChange const*");
