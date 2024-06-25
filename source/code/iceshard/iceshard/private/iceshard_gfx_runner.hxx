/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "gfx/iceshard_gfx_device.hxx"
#include <ice/shard_container.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/sync_manual_events.hxx>

namespace ice::gfx
{

    enum class IceshardGfxRunnerState : ice::u8
    {
        Fresh,
        Resumed,
        SwapchainDirty,
        Suspended,
        Shutdown,
    };

    enum class IceshardGfxWorldState : ice::u8
    {
        Ignored,
        Inactive,
        Active,
    };

    struct IceshardGfxRunner
        : public ice::gfx::GfxRunner
        , public ice::EngineStateCommitter
    {
        IceshardGfxRunner(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::gfx::GfxContext> gfx_ctx,
            ice::gfx::GfxRunnerCreateInfo const& create_info
        ) noexcept;
        ~IceshardGfxRunner() noexcept override;

        void update_rendergraph(ice::UniquePtr<ice::gfx::GfxGraphRuntime> rendergraph) noexcept override;

        void update() noexcept;

        auto draw_frame(
            ice::EngineFrame const& frame,
            ice::Clock const& clock
        ) noexcept -> ice::Task<> override;

        auto context() noexcept -> ice::gfx::GfxContext& override;

        void destroy() noexcept;

    public: // Implementation of: ice::gfx::GfxRunner
        void on_resume() noexcept override { }
        void on_suspend() noexcept override { }

    public: // Implementation of: ice::EngineStateCommitter
        bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept override;

    private:
        ice::Allocator& _alloc;
        ice::Engine& _engine;
        ice::UniquePtr<ice::gfx::GfxContext> _context;

        ice::u8 _flow_id;
        ice::TaskQueue _queue, _queue_transfer, _queue_end;
        ice::TaskScheduler& _scheduler;
        ice::render::RenderFence* _present_fence;

        ice::UniquePtr<ice::gfx::GfxStageRegistry> _stages;

        // ice::HashMap<ice::gfx::GfxStage const*> _stages;
        ice::gfx::IceshardGfxRunnerState _state;
        ice::HashMap<ice::gfx::IceshardGfxWorldState> _world_states;
        ice::UniquePtr<ice::gfx::GfxGraphRuntime> _rendergraph;

        ice::ScopedTaskContainer _gfx_tasks;

        ice::ManualResetEvent _gfx_frame_finished;
    };

} // namespace ice
