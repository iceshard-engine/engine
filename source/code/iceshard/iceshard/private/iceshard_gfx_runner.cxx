/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_runner.hxx"
#include "gfx/iceshard_gfx_device.hxx"
#include <ice/gfx/gfx_stage.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>

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

    void gather_and_execute_tasks(
        ice::StringID_Hash world_name,
        ice::WorldUpdater& updater,
        ice::Shard shard,
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        updater.force_update({ world_name }, shard, out_tasks);
        ICE_ASSERT(
            ice::u8_max >= ice::array::count(out_tasks),
            "Gathered more tasks than it's possible to track!"
        );
    }

    IceshardGfxRunner::IceshardGfxRunner(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::gfx::GfxDevice> gfx_device,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept
        : _alloc{ alloc }
        , _engine{ create_info.engine }
        , _device{ ice::move(gfx_device) }
        , _barrier{ }
        , _barrier_state{ }
        , _queue{ }
        , _queue_transfer{ }
        , _queue_end{ }
        , _scheduler{ _queue }
        , _thread{ }
        , _present_fence{ _device->device().create_fence() }
        , _stages{ _alloc }
        , _state{ IceshardGfxRunnerState::SwapchainDirty }
        , _world_states{ _alloc }
    {
        ice::TaskThreadInfo const info{
            .exclusive_queue = true,
            .debug_name = "ice.gfx",
        };
        _thread = ice::create_thread(_alloc, _queue, info);
    }

    IceshardGfxRunner::~IceshardGfxRunner() noexcept
    {

        _device->device().destroy_fence(_present_fence);
    }

    void IceshardGfxRunner::on_resume() noexcept
    {
        //IPT_ZONE_SCOPED;

        //GfxStateChange const gfx_state_change{
        //    .assets = _engine.assets(),
        //    .device = *_device,
        //};

        //// TODO: Allow to set the number of tasks during world creation.
        //static ice::StackAllocator_2048 alloc{ };
        //alloc.reset();

        //ice::Array<ice::Task<>, ice::ContainerLogic::Complex> tasks{ alloc };
        //ice::ucount constexpr max_capcity = ice::mem_max_capacity<ice::Task<>>(ice::StackAllocator_2048::Constant_InternalCapacity);
        //ice::array::set_capacity(tasks, max_capcity);

        //{
        //    IPT_ZONE_SCOPED_NAMED("gfx_gather_tasks");
        //    ice::WorldUpdater& updater = _engine.worlds_updater();
        //    updater.update(ice::shard(ShardID_GfxStartup, &gfx_state_change), tasks);
        //}
        //ICE_ASSERT(
        //    ice::u8_max >= ice::array::count(tasks),
        //    "Gathered more tasks than it's possible to track!"
        //);

        //_barrier.reset(ice::u8(ice::array::count(tasks)));
        //{
        //    IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");
        //    ice::manual_wait_for_all(tasks, _barrier);

        //    // Wait for tasks to finish.
        //    _barrier.wait();
        //}
    }

    void IceshardGfxRunner::update_states(
        ice::ShardContainer const& shards,
        GfxStateChange const& gfx_state,
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        auto const update_state = [this, &gfx_state, &out_tasks](ice::StringID_Hash name, IceshardGfxWorldState state) noexcept
            {
                IceshardGfxWorldState old_state = ice::hashmap::get(
                    _world_states, ice::hash(name), IceshardGfxWorldState::Ignored
                );

                if (state == IceshardGfxWorldState::Active)
                {
                    if (old_state == IceshardGfxWorldState::Ignored)
                    {
                        gather_and_execute_tasks(
                            name, _engine.worlds_updater(), ShardID_GfxStartup | &gfx_state, out_tasks
                        );
                        old_state = IceshardGfxWorldState::Inactive;
                    }

                    if (old_state == IceshardGfxWorldState::Inactive)
                    {
                        gather_and_execute_tasks(
                            name, _engine.worlds_updater(), ShardID_GfxResume | &gfx_state, out_tasks
                        );
                        ice::hashmap::set(_world_states, ice::hash(name), state);
                    }
                }
                else if (state == IceshardGfxWorldState::Ignored)
                {
                    if (old_state == IceshardGfxWorldState::Active)
                    {
                        gather_and_execute_tasks(
                            name, _engine.worlds_updater(), ShardID_GfxSuspend | &gfx_state, out_tasks
                        );
                        old_state = IceshardGfxWorldState::Inactive;
                    }

                    if (old_state == IceshardGfxWorldState::Inactive)
                    {
                        gather_and_execute_tasks(
                            name, _engine.worlds_updater(), ShardID_GfxShutdown | &gfx_state, out_tasks
                        );
                        ice::hashmap::set(_world_states, ice::hash(name), state);
                    }
                }
                else
                {
                    ICE_ASSERT_CORE(false);
                }
            };

        for (ice::Shard shard : shards)
        {
            switch (shard.id.name.value)
            {
            case ice::ShardID_WorldActivated.name.value:
                update_state(ice::shard_shatter<ice::StringID_Hash>(shard, ice::StringID_Invalid), IceshardGfxWorldState::Active);
                break;
            case ice::ShardID_WorldDeactivated.name.value:
                update_state(ice::shard_shatter<ice::StringID_Hash>(shard, ice::StringID_Invalid), IceshardGfxWorldState::Ignored);
                break;
            default:
                break;
            }
        }

        return;
    }

    auto IceshardGfxRunner::draw_frame(
        ice::EngineFrame const& frame,
        ice::gfx::GfxGraphRuntime& graph_runtime,
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
            .device = *_device,
            .stages = *this,
        };

        // We create once and always reset the object next time
        static ice::StackAllocator_2048 alloc{ };
        alloc.reset();
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex> tasks{ alloc };
        // TODO: Allow to set the number of tasks during world creation.
        ice::ucount constexpr max_capcity = ice::mem_max_capacity<ice::Task<>>(ice::StackAllocator_2048::Constant_InternalCapacity);
        ice::array::set_capacity(tasks, max_capcity);

        GfxStateChange const gfx_state{
            .assets = _engine.assets(),
            .device = *_device,
            .renderpass = graph_runtime.renderpass(),
            .frame_transfer = { _queue_transfer },
            .frame_end = { _queue_end },
        };

        update_states(
            frame.shards(), gfx_state, tasks
        );

        _barrier_state.reset(ice::u8(ice::array::count(tasks)));
        {
            IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");
            ice::manual_wait_for_all(tasks, _barrier_state);

            v2::GfxQueueGroup_Temp& group = _device->queue_group(0);
            ice::gfx::GfxQueue* queue;
            bool const queue_exists = group.get_queue(ice::render::QueueFlags::Transfer, queue);
            ICE_ASSERT_CORE(queue_exists);
            queue->reset();

            ice::render::CommandBuffer transfer_buffer;
            queue->request_command_buffers(render::CommandBufferType::Primary, { &transfer_buffer , 1 });
            _device->device().get_commands().begin(transfer_buffer);

            bool has_work = false;
            //while (_barrier.is_set() == false)
            {
                for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_queue_transfer._awaitables))
                {
                    awaitable->result.ptr = &transfer_buffer;
                    awaitable->_coro.resume();
                    has_work = true;
                }
            }

            // TODO: Log how many tasks are still around
            _device->device().get_commands().end(transfer_buffer);

            // Wait for tasks to finish.
            //_barrier.wait();

            if (has_work)
            {
                IPT_ZONE_SCOPED_NAMED("gfx_transfer_commands");
                _present_fence->reset();
                queue->submit_command_buffers({ &transfer_buffer, 1 }, _present_fence);

                // Wait for the fence to finish work
                _present_fence->wait(10'000'000);
            }
        }


        {
            ice::hashmap::clear(_stages);

            IPT_ZONE_SCOPED_NAMED("gfx_gather_tasks");
            ice::WorldUpdater& updater = _engine.worlds_updater();

            ice::array::clear(tasks);
            updater.update(ice::shard(ShardID_GfxFrameUpdate, &gfx_update), tasks);
        }
        ICE_ASSERT(
            ice::u8_max >= ice::array::count(tasks),
            "Gathered more tasks than it's possible to track!"
        );

        _barrier.reset(ice::u8(ice::array::count(tasks)));
        {
            IPT_ZONE_SCOPED_NAMED("gfx_await_tasks");
            ice::manual_wait_for_all(tasks, _barrier);
        }

        while (_barrier.is_set() == false || _barrier_state.is_set() == false)
        {
            for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_queue_end._awaitables))
            {
                awaitable->_coro.resume();
            }
        }

        _barrier.wait();
        _barrier_state.wait();

        // Execute all stages (currently done simply going over the render graph)
        _present_fence->reset();

        if (graph_runtime.execute(frame, *this, *_present_fence))
        {
            _present_fence->wait(10'000'000);
        }
        co_return;
    }

    void IceshardGfxRunner::on_suspend() noexcept
    {
    }

    void IceshardGfxRunner::final_update(
        ice::EngineFrame const& frame,
        ice::gfx::GfxGraphRuntime& graph_runtime,
        ice::Clock const& clock
    ) noexcept
    {
        ice::TaskQueue queue;
        GfxStateChange const gfx_state{
            .assets = _engine.assets(),
            .device = *_device,
            .renderpass = graph_runtime.renderpass(),
            .frame_transfer = { queue },
            .frame_end = { queue },
        };

        // We create once and always reset the object next time
        static ice::StackAllocator_2048 alloc{ };
        alloc.reset();
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex> tasks{ alloc };
        // TODO: Allow to set the number of tasks during world creation.
        ice::ucount constexpr max_capcity = ice::mem_max_capacity<ice::Task<>>(ice::StackAllocator_2048::Constant_InternalCapacity);
        ice::array::set_capacity(tasks, max_capcity);

        update_states(
            frame.shards(), gfx_state, tasks
        );

        ICE_ASSERT_CORE(ice::linked_queue::empty(queue._awaitables));

        ice::wait_for_all(ice::array::slice(tasks));
    }

    auto IceshardGfxRunner::device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_device;
    }

    void IceshardGfxRunner::destroy() noexcept
    {
        _alloc.destroy(this);
    }

    void IceshardGfxRunner::add_stage(
        ice::StringID_Arg name,
        ice::gfx::GfxStage const* stage
    ) noexcept
    {
        ice::multi_hashmap::insert(_stages, ice::hash(name), stage);
    }

    void IceshardGfxRunner::execute_stages(
        ice::EngineFrame const& frame,
        ice::StringID_Arg name,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& render_api
    ) const noexcept
    {
        IPT_ZONE_SCOPED;
        auto it = ice::multi_hashmap::find_first(_stages, ice::hash(name));
        while (it != nullptr)
        {
            it.value()->draw(frame, cmds, render_api);
            it = ice::multi_hashmap::find_next(_stages, it);
        }
    }

} // namespace ice::gfx
