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

    struct IceshardGfxRunner : public ice::gfx::GfxRunner
    {
        IceshardGfxRunner(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::gfx::GfxDevice> gfx_device,
            ice::gfx::GfxRunnerCreateInfo const& create_info
        ) noexcept;
        ~IceshardGfxRunner() noexcept override;

        void update_rendergraph(ice::UniquePtr<ice::gfx::GfxGraphRuntime> rendergraph) noexcept override
        {
            _rendergraph = ice::move(rendergraph);
        }

        void update_states(ice::ShardContainer const& shards) noexcept;

        auto draw_frame(
            ice::gfx::GfxOperationParams const& params
        ) noexcept -> ice::Task<> override;

        auto device() noexcept -> ice::gfx::GfxDevice& override;

        void destroy() noexcept;

    private:
        ice::Allocator& _alloc;
        ice::Engine& _engine;
        ice::UniquePtr<ice::gfx::GfxDevice> _device;

        ice::u8 _flow_id;
        ice::ScopedTaskContainer _tasks_container;
        ice::TaskQueue _queue, _queue_transfer, _queue_end;
        ice::TaskScheduler _scheduler;
        ice::UniquePtr<ice::TaskThread> _thread;
        ice::render::RenderFence* _present_fence;

        ice::UniquePtr<ice::gfx::GfxStageRegistry> _stages;

        // TODO: Replace with proper task completion tracker.
        std::atomic_uint32_t _gfx_tasks;

        // ice::HashMap<ice::gfx::GfxStage const*> _stages;
        ice::gfx::IceshardGfxRunnerState _state;
        ice::HashMap<ice::gfx::IceshardGfxWorldState> _world_states;
        ice::UniquePtr<ice::gfx::GfxGraphRuntime> _rendergraph;

        alignas(alignof(ice::gfx::GfxStateChange)) char _params_storage[sizeof(ice::gfx::GfxStateChange)];
    };

} // namespace ice
