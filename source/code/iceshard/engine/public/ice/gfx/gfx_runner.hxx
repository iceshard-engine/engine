/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_types.hxx>
#include <ice/clock.hxx>
#include <ice/container/array.hxx>
#include <ice/engine_types.hxx>
#include <ice/module_register.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/stringid.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/gfx/ice_gfx_render_graph_v3.hxx>

namespace ice::gfx
{

    class GfxTrait;
    class GfxDevice;
    class GfxFrame;

    class GfxRunner
    {
    public:
        virtual ~GfxRunner() noexcept = default;

        virtual auto aquire_world() noexcept -> ice::World* = 0;
        virtual void release_world(ice::World* world) noexcept = 0;

        virtual void set_event(ice::ManualResetEvent* event) noexcept = 0;

        virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
        virtual auto frame() noexcept -> ice::gfx::GfxFrame& = 0;

        virtual void draw_frame(ice::EngineFrame const& engine_frame) noexcept = 0;
    };

    namespace v2
    {

        struct QueueDefinition
        {
            ice::StringID name;
            ice::render::QueueFlags flags;
        };

        struct GfxRunnerCreateInfo
        {
            ice::Engine& engine;
            ice::render::RenderDriver& driver;
            ice::render::RenderSurface& surface;
            ice::Span<ice::gfx::v2::QueueDefinition const> render_queues;
        };

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
            ice::gfx::v3::GfxStageRegistry& stages;
        };

        struct GfxRunner
        {
            virtual ~GfxRunner() noexcept = default;

            virtual void on_resume() noexcept { }

            virtual auto draw_frame(
                ice::EngineFrame const& frame,
                ice::gfx::v3::GfxGraphRuntime& render_graph,
                ice::Clock const& clock
            ) noexcept -> ice::Task<> = 0;

            virtual void on_suspend() noexcept { }

            virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
        };

        auto create_gfx_runner(
            ice::Allocator& alloc,
            ice::ModuleRegister& registry,
            ice::gfx::v2::GfxRunnerCreateInfo const& create_info
        ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRunner>;

        static constexpr ice::ShardID ShardID_GfxStartup = "event/gfx/startup`ice::gfx::v2::GfxStateChange const*"_shardid;
        static constexpr ice::ShardID ShardID_GfxResume = "event/gfx/resume`ice::gfx::v2::GfxStateChange const*"_shardid;
        static constexpr ice::ShardID ShardID_GfxSwapchainReset = "event/gfx/swapchain-reset`ice::gfx::v2::GfxStateChange const*"_shardid;
        static constexpr ice::ShardID ShardID_GfxSuspend = "event/gfx/suspend`ice::gfx::v2::GfxStateChange const*"_shardid;
        static constexpr ice::ShardID ShardID_GfxShutdown = "event/gfx/shutdown`ice::gfx::v2::GfxStateChange const*"_shardid;

        static constexpr ice::ShardID ShardID_GfxFrameUpdate = "event/gfx/frame-update`ice::gfx::v2::GfxFrameUpdate const*"_shardid;

    } // namespace v2

} // namespace ice::gfx

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::v2::GfxFrameUpdate const*> = ice::shard_payloadid("ice::gfx::v2::GfxFrameUpdate const*");

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::gfx::v2::GfxStateChange const*> = ice::shard_payloadid("ice::gfx::v2::GfxStateChange const*");
