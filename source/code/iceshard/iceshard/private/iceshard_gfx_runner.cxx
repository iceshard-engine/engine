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

    auto create_gfx_runner_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept -> ice::gfx::GfxRunner*
    {
        ice::UniquePtr<ice::gfx::GfxDevice> gfx_device = ice::gfx::create_graphics_device(
            alloc,
            create_info.driver,
            create_info.surface,
            create_info.render_queues
        );

        if (gfx_device != nullptr)
        {
            return alloc.create<ice::gfx::IceshardGfxRunner>(alloc, ice::move(gfx_device), create_info);
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

        auto gfx_state_shards(void* userdata, ice::ShardContainer& out_shards) noexcept
        {
            ice::gfx::GfxStateChange const* params = reinterpret_cast<ice::gfx::GfxStateChange const*>(userdata);
            ice::Shard const shards[]{
                { ShardID_GfxStartup | params },
                { ShardID_GfxShutdown | params },
            };

            ice::shards::push_back(out_shards, shards);
        }

        //auto register_gfx_activation_flow(void* userdata, ice::WorldStateTracker& world_states) noexcept -> ice::u8
        //{
        //    ice::u8 const world_activity_flow = world_states.flowid("world-activity"_sid);

        //    ice::StackAllocator_1024 temp_alloc;
        //    ice::Array<ice::WorldStateStage> stages{ temp_alloc };
        //    ice::array::push_back(stages,
        //        ice::WorldStateStage{
        //            .trigger = ice::ShardID_WorldActivated,
        //            .required_state = 0,
        //            .resulting_state = 1,
        //            .dependent_state = { .flow_value = 2, .flow_id = world_activity_flow },
        //            .event_shard = ShardID_GfxStartup
        //        }
        //    );

        //    ice::array::push_back(stages,
        //        ice::WorldStateStage{
        //            .trigger = ice::ShardID_WorldDeactivate,
        //            .required_state = 1,
        //            .resulting_state = 0,
        //            .blocked_state = {.flow_value = 2, .flow_id = world_activity_flow },
        //            .event_shard = ShardID_GfxShutdown
        //        }
        //    );

        //    return world_states.register_flow(
        //        {
        //            .name = "gfx-activity"_sid,
        //            .stages = stages,
        //            .userdata = userdata,
        //            .fn_shards = gfx_state_shards,
        //        }
        //    );
        //}

    } // namespace detail

    IceshardGfxRunner::IceshardGfxRunner(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::gfx::GfxDevice> gfx_device,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept
        : _alloc{ alloc }
        , _engine{ create_info.engine }
        , _device{ ice::move(gfx_device) }
        , _tasks_container{ _alloc }
        , _queue{ }
        , _queue_transfer{ }
        , _queue_end{ }
        , _scheduler{ _queue }
        , _thread{ }
        , _present_fence{ _device->device().create_fence() }
        , _stages{ ice::gfx::create_stage_registry(_alloc) }
        , _state{ IceshardGfxRunnerState::SwapchainDirty }
        , _world_states{ _alloc }
    {
        ice::TaskThreadInfo const info{
            .exclusive_queue = true,
            .debug_name = "ice.gfx",
        };
        _thread = ice::create_thread(_alloc, _queue, info);

        ice::EngineStateTrigger triggers[]{ detail::StateTrigger_ActivateRuntime, detail::StateTrigger_DeactivateRuntime };
        _engine.states().register_graph(
            { .initial = detail::State_GfxInactive, .commiter = this, .enable_subname_states = true },
            triggers
        );
        //_flow_id = detail::register_gfx_activation_flow(_params_storage, _engine.worlds_states());
    }

    IceshardGfxRunner::~IceshardGfxRunner() noexcept
    {

        _device->device().destroy_fence(_present_fence);
    }

    auto IceshardGfxRunner::draw_frame(
        ice::gfx::GfxOperationParams const& params
    ) noexcept -> ice::Task<>
    {
        //// Initialize state for new worlds
        //ice::shards::for_each(
        //    params.frame.shards(),
        //    ice::ShardID_WorldCreated,
        //    [this](ice::Shard shard) noexcept
        //    {
        //        ice::StringID_Hash worldname;
        //        if (ice::shard_inspect(shard, worldname))
        //        {
        //            _engine.states().initialize_subname_state(detail::StateGraph_GfxRuntime, { worldname });
        //        }
        //    }
        //);

        co_await _scheduler;

        IPT_ZONE_SCOPED;
        IPT_FRAME_MARK_NAMED("Graphics");

        GfxFrameUpdate const gfx_update{
            .clock = params.clock,
            .assets = _engine.assets(),
            .frame = params.frame,
            .device = *_device,
        };

        update_states(frame.shards());

        ice::gfx::GfxStages gpu_stages{
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end }
        };

        if (graph_runtime.prepare(gpu_stages, *_stages, new_tasks))
        {
            ice::resume_tasks_on(new_tasks, _scheduler);
        }

        if (ice::linked_queue::any(_queue_transfer._awaitables))
        {
            IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");

            v2::GfxQueueGroup_Temp& group = _device->queue_group(0);
            ice::gfx::GfxQueue* queue;
            bool const queue_exists = group.get_queue(ice::render::QueueFlags::Transfer, queue);
            ICE_ASSERT_CORE(queue_exists);
            queue->reset();

            ice::render::CommandBuffer transfer_buffer;
            queue->request_command_buffers(render::CommandBufferType::Primary, { &transfer_buffer , 1 });
            _device->device().get_commands().begin(transfer_buffer);

            // We only process one for this queue.
            bool const has_work = _queue_transfer.process_all(&transfer_buffer) > 0;

            // TODO: Log how many tasks are still around
            _device->device().get_commands().end(transfer_buffer);

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
        while(_tasks_container.running_tasks() > 0)
        {
            _queue_end.process_all();
        }

        // Execute all stages (currently done simply going over the render graph)
        _present_fence->reset();

        if (_rendergraph->execute(params.frame, *this, *_present_fence))
        {
            _present_fence->wait(10'000'000);
        }
        co_return;
    }


    void IceshardGfxRunner::update_states(
        ice::WorldStateTracker& state_tracker,
        ice::gfx::GfxOperationParams const& params
    ) noexcept
    {
        ice::gfx::GfxStages gpu_stages{
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end }
        };
        auto s = ice::gfx::create_stage_registry(_alloc);

        ice::Array<ice::Task<>> new_tasks{_alloc};
        if (graph_runtime.prepare(gpu_stages, *s, new_tasks))
        {
            ice::wait_for_all(new_tasks);
        }

        update_states(frame.shards());
    }

    auto IceshardGfxRunner::device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_device;
    }

    void IceshardGfxRunner::destroy() noexcept
    {
        _alloc.destroy(this);
    }

} // namespace ice::gfx
