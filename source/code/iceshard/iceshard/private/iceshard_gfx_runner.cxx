/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_runner.hxx"
#include "iceshard_runner.hxx"
#include "gfx/iceshard_gfx_device.hxx"
#include <ice/gfx/gfx_stage.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/engine_state_definition.hxx>

#include <ice/task.hxx>
#include <ice/task_utils.hxx>

#include <ice/mem_allocator_stack.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/module_register.hxx>
#include <ice/profiler.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_frame.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

    static constexpr ice::u64 Constant_NanoSecondsInMicroSeconds = 1000;
    static constexpr ice::u64 Constant_NanoSecondsInMiliSeconds = 1000 * Constant_NanoSecondsInMicroSeconds;
    static constexpr ice::u64 Constant_NanoSecondsInSeconds = 1000 * Constant_NanoSecondsInMiliSeconds;

    auto create_gfx_runner_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept -> ice::gfx::GfxRunner*
    {
        ice::UniquePtr<ice::gfx::GfxContext> gfx_ctx = ice::gfx::create_graphics_device(
            alloc,
            create_info.driver,
            create_info.surface,
            create_info.render_queues
        );

        if (gfx_ctx != nullptr)
        {
            return alloc.create<ice::gfx::IceshardGfxRunner>(alloc, ice::move(gfx_ctx), create_info);
        }
        else
        {
            return {};
        }
    }

    void destroy_gfx_runner_fn(ice::gfx::GfxRunner* runner) noexcept
    {
        static_cast<ice::gfx::IceshardGfxRunner*>(runner)->destroy();
    }

} // namespace ice

namespace ice::gfx
{

    namespace detail
    {

        static constexpr ice::EngineStateGraph StateGraph_GfxRuntime = "gfx-runtime"_state_graph;
        static constexpr ice::EngineState State_GfxActive = StateGraph_GfxRuntime | "active";
        static constexpr ice::EngineState State_GfxInactive = StateGraph_GfxRuntime | "inactive";

        static constexpr ice::EngineStateTrigger StateTrigger_ActivateRuntime{
            .when = ShardID_WorldRuntimeActivated, // TODO: Add na 'after' event
            .from = State_GfxInactive,
            .to = State_GfxActive,
        };
        static constexpr ice::EngineStateTrigger StateTrigger_DeactivateRuntime{
            .before = State_WorldRuntimeInactive, // Parent state graph
            .from = State_GfxActive,
            .to = State_GfxInactive,
        };

    } // namespace detail

    IceshardGfxRunner::IceshardGfxRunner(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::gfx::GfxContext> gfx_ctx,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept
        : _alloc{ alloc }
        , _engine{ create_info.engine }
        , _context{ ice::move(gfx_ctx) }
        , _tasks_container{ _alloc }
        , _queue_transfer{ }
        , _queue_end{ }
        , _scheduler{ create_info.gfx_thread }
        , _present_fence{ _context->device().create_fence() }
        , _stages{ ice::gfx::create_stage_registry(_alloc) }
        , _state{ IceshardGfxRunnerState::SwapchainDirty }
        , _world_states{ _alloc }
        , _gfx_tasks{ _alloc }
    {
        ice::EngineStateTrigger triggers[]{ detail::StateTrigger_ActivateRuntime, detail::StateTrigger_DeactivateRuntime };
        _engine.states().register_graph(
            { .initial = detail::State_GfxInactive, .commiter = this, .enable_subname_states = true },
            triggers
        );
    }

    IceshardGfxRunner::~IceshardGfxRunner() noexcept
    {

        _context->device().destroy_fence(_present_fence);
    }

    auto IceshardGfxRunner::draw_frame(
        ice::EngineFrame const& frame,
        ice::Clock const& clock
    ) noexcept -> ice::Task<>
    {
        co_await _scheduler;

        IPT_ZONE_SCOPED;
        IPT_FRAME_MARK_NAMED("Graphics");

        GfxFrameUpdate const gfx_update{
            .clock = clock,
            .assets = _engine.assets(),
            .frame = frame,
            .context = *_context,
        };

        ice::Shard shards[]{ ice::gfx::ShardID_GfxFrameUpdate | &gfx_update };
        _engine.worlds_updater().update(_gfx_tasks, { shards });
        _gfx_tasks.execute_tasks();
        _gfx_tasks.wait_tasks();

        ice::gfx::GfxFrameStages gpu_stages{
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end }
        };

        if (_rendergraph->prepare(gpu_stages, *_stages, _gfx_tasks))
        {
            _gfx_tasks.execute_tasks_detached(_scheduler);
        }

        if (ice::linked_queue::any(_queue_transfer._awaitables) || _gfx_tasks.running_tasks() > 0)
        {
            IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");

            v2::GfxQueueGroup_Temp& group = _context->queue_group(0);
            ice::gfx::GfxQueue* queue;
            bool const queue_exists = group.get_queue(ice::render::QueueFlags::Transfer, queue);
            ICE_ASSERT_CORE(queue_exists);
            queue->reset();

            ice::render::CommandBuffer transfer_buffer;
            queue->request_command_buffers(render::CommandBufferType::Primary, { &transfer_buffer , 1 });
            _context->device().get_commands().begin(transfer_buffer);

            // We only process one for this queue.
            bool const has_work = _queue_transfer.process_all(&transfer_buffer) > 0;

            // TODO: Log how many tasks are still around
            _context->device().get_commands().end(transfer_buffer);

            if (has_work)
            {
                IPT_ZONE_SCOPED_NAMED("gfx_transfer_commands");
                _present_fence->reset();
                queue->submit_command_buffers({ &transfer_buffer, 1 }, _present_fence);

                // Wait for the fence to finish work
                _present_fence->wait(100'000'000);
            }
        }

        _tasks_container.execute_tasks();

        // Execute all stages (currently done simply going over the render graph)
        _present_fence->reset();

        if (_rendergraph->execute(frame, *_present_fence))
        {
            _present_fence->wait(10'000'000);
        }

        while(ice::linked_queue::any(_queue_end._awaitables) || _tasks_container.running_tasks() > 0)
        {
            _queue_end.process_all();
        }
        co_return;
    }

    auto IceshardGfxRunner::context() noexcept -> ice::gfx::GfxContext&
    {
        return *_context;
    }

    void IceshardGfxRunner::destroy() noexcept
    {
        _alloc.destroy(this);
    }

    bool IceshardGfxRunner::commit(
        ice::EngineStateTrigger const& trigger,
        ice::Shard trigger_shard,
        ice::ShardContainer& out_shards
    ) noexcept
    {
        ice::gfx::GfxStateChange const params{
            .assets = _engine.assets(),
            .context = *_context,
            .stages = *_stages
        };

        ice::StringID_Hash world_name;
        if (ice::shard_inspect(trigger_shard, world_name) == false)
        {
            return false;
        }

        ice::Shard shards[1];
        if (trigger.to == detail::State_GfxActive)
        {
            shards[0] = ice::shard(ice::gfx::ShardID_GfxStartup, &params);
        }
        else if (trigger.to == detail::State_GfxInactive)
        {
            ice::gfx::GfxFrameStages gpu_stages{
                .frame_transfer = { _queue_transfer },
                .frame_end = { _queue_end }
            };

            auto s = ice::gfx::create_stage_registry(_alloc);
            {
                if (_rendergraph->prepare(gpu_stages, *s, _gfx_tasks))
                {
                    _gfx_tasks.execute_tasks();
                }
            }

            shards[0] = ice::shard(ice::gfx::ShardID_GfxShutdown, &params);
        }

        ice::ScopedTaskContainer tasks{ _alloc };
        _engine.worlds_updater().update(tasks, shards);
        return true;
    }

} // namespace ice::gfx
