/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_runner.hxx"
#include "iceshard_runner.hxx"
#include "gfx/iceshard_gfx_device.hxx"

#include <ice/container/hashmap.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_params.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/module_register.hxx>
#include <ice/profiler.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/world/world_updater.hxx>

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
            { .initial = detail::State_GfxInactive, .committer = this, .enable_subname_states = true },
            triggers
        );
        _gfx_frame_finished.set();
    }

    IceshardGfxRunner::~IceshardGfxRunner() noexcept
    {
        _context->device().destroy_fence(_present_fence);
    }

    void IceshardGfxRunner::update_rendergraph(ice::UniquePtr<ice::gfx::GfxGraphRuntime> rendergraph) noexcept
    {
        if (_rendergraph != nullptr)
        {
            ice::gfx::GfxFrameStages gpu_stages{
                .scheduler = _scheduler,
                .frame_transfer = { _queue_transfer },
                .frame_end = { _queue_end }
            };

            auto stage_registry = ice::gfx::create_stage_registry(_alloc);
            {
                if (_rendergraph->prepare(gpu_stages, *stage_registry, _gfx_tasks))
                {
                    _gfx_tasks.execute_tasks();
                }
            }
        }

        _rendergraph = ice::move(rendergraph);
    }

    auto IceshardGfxRunner::update_data(
        ice::EngineFrame& frame,
        ice::Clock const& clock
    ) noexcept -> ice::Task<>
    {
        //IPT_ZONE_SCOPED;
        ice::TaskContainer& current_tasks = frame.tasks_container();

        GfxTaskParams const task_params{
            { TraitTaskType::Graphics },
            clock,
            _engine.assets().resources(),
            _engine.assets(),
            _scheduler,
            *_context
        };

        GfxFrameUpdate const gfx_update{
            .frame = frame,
            .context = *_context,
        };

        ice::WorldUpdater& world_updater = _engine.worlds_updater();
        {
            ice::Shard shards[]{ ice::gfx::ShardID_GfxFrameUpdate | &gfx_update };

            IPT_ZONE_SCOPED_NAMED("gather_tasks");
            world_updater.update(current_tasks, task_params, frame.shards());
            world_updater.update(current_tasks, task_params, { shards });
        }

        co_await current_tasks.await_tasks_scheduled_on(_scheduler, _scheduler);
    }

    auto IceshardGfxRunner::draw_frame(
        ice::EngineFrame const& frame,
        ice::Clock const& clock
    ) noexcept -> ice::Task<>
    {
        co_await _scheduler;

        //IPT_ZONE_SCOPED;
        IPT_FRAME_MARK_NAMED("Graphics");
        RenderTaskParams task_params{
            .clock = clock,
            .resources = _engine.assets().resources(),
            .assets = _engine.assets(),
            .scheduler = _scheduler,
            .gfx = *_context
        };
        task_params.task_type = TraitTaskType::Render;

        GfxFrameStages gpu_stages{
            .scheduler = _scheduler,
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end }
        };

        RenderFrameUpdate const gfx_update{
            .frame = frame,
            .context = *_context,
            .stages = gpu_stages,
        };

        ice::Shard shards[]{ ice::gfx::ShardID_RenderFrameUpdate | &gfx_update };
        _engine.worlds_updater().update(_gfx_tasks, task_params, { shards });

        //TODO: Currently the container should be "dead" after it's await scheduled on
        {
            ice::Array<ice::Task<>> tasks = _gfx_tasks.extract_tasks();
            //co_await ice::await_scheduled(tasks, _scheduler);
            ice::execute_tasks(tasks);
        }

        if (_rendergraph->prepare(gpu_stages, *_stages, _gfx_tasks))
        {
            IPT_ZONE_SCOPED_NAMED("gfx_resume_prepare_tasks");
            // The tasks here might take more than a frame to finish. We should track them somewhere else.
            ice::Array<ice::Task<>> tasks = _gfx_tasks.extract_tasks();
            ice::execute_tasks(tasks);
        }

        if (_queue_transfer.any() || _gfx_tasks.running_tasks() > 0)
        {
            IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");

            v2::GfxQueueGroup_Temp& group = _context->queue_group(0);
            ice::gfx::GfxQueue* queue;
            // Graphics Queues can accept TransferQueue work
            bool const queue_exists = group.get_queue(ice::render::QueueFlags::Graphics, queue);
            ICE_ASSERT_CORE(queue_exists);
            queue->reset();

            ice::render::CommandBuffer transfer_buffer;
            {
                IPT_ZONE_SCOPED_NAMED("gfx_request_transfer_command_buffers");
                queue->request_command_buffers(render::CommandBufferType::Primary, { &transfer_buffer , 1 });
                _context->device().get_commands().begin(transfer_buffer);
            }

            // Resumes all queued awaitables on the scheduler with this thread as the final awaitable.
            bool const has_work = co_await ice::await_scheduled_queue(_queue_transfer, &transfer_buffer, _scheduler);

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

        // Execute all stages (currently done simply going over the render graph)
        _present_fence->reset();

        if (_rendergraph->execute(frame, *_present_fence))
        {
            _rendergraph->present();
        }

        co_await ice::await_scheduled_queue(_queue_end, _scheduler);
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
        IPT_ZONE_SCOPED;
        RenderTaskParams task_params{
            .clock = {},
            .resources = _engine.assets().resources(),
            .assets = _engine.assets(),
            .scheduler = _scheduler,
            .gfx = *_context
        };
        task_params.task_type = TraitTaskType::Render;

        ice::gfx::GfxFrameStages gpu_stages{
            .scheduler = _scheduler,
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end }
        };

        ice::gfx::GfxStateChange const params{
            .stages = gpu_stages,
            .registry = *_stages
        };

        ice::StringID world_name;
        if (ice::shard_inspect(trigger_shard, world_name.value) == false)
        {
            return false;
        }

        ice::Shard shards[1];
        ice::Task<> statetask;
        if (trigger.to == detail::State_GfxActive)
        {
            shards[0] = ice::shard(ice::gfx::ShardID_GfxStartup, &params);

            ice::ScopedTaskContainer tasks{ _alloc };
            _engine.worlds_updater().update(world_name, tasks, task_params, shards);

            ice::wait_for(tasks.await_tasks_scheduled_on(_scheduler, _scheduler));
        }
        else if (trigger.to == detail::State_GfxInactive)
        {
            ice::ScopedTaskContainer tasks{ _alloc };
            {
                shards[0] = ice::shard(ice::gfx::ShardID_GfxShutdown, &params);

                _engine.worlds_updater().update(world_name, tasks, task_params, shards);
            }

            auto const gpu_graph_task = [](
                ice::ScopedTaskContainer& pretasks,
                ice::gfx::IceshardGfxRunner& self,
                ice::gfx::GfxStateChange const& params,
                ice::gfx::GfxFrameStages& gpu_stages,
                ice::gfx::GfxStageRegistry& stages
            ) noexcept -> ice::Task<>
            {
                co_await pretasks.await_tasks_scheduled_on(gpu_stages.scheduler, gpu_stages.scheduler);

                co_await gpu_stages.scheduler;

                if (self._rendergraph->prepare(gpu_stages, stages, self._gfx_tasks))
                {
                    ice::Array<ice::Task<>> tasks = self._gfx_tasks.extract_tasks();
                    co_await ice::await_tasks(tasks);
                }
            };

            ice::wait_for(gpu_graph_task(tasks, *this, params, gpu_stages, *_stages));
        }

        return true;
    }

} // namespace ice::gfx
