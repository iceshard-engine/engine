/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_world.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/engine_module.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_devui.hxx>
#include <ice/task_utils.hxx>
#include <ice/profiler.hxx>


namespace ice
{

    namespace detail
    {

        static constexpr ice::EngineStateTrigger StateTrigger_ActivateRuntime{
            .when = ShardID_WorldActivated,
            .from = State_WorldRuntimeInactive,
            .to = State_WorldRuntimeActive,
            .results = ShardID_WorldRuntimeActivated
        };
        static constexpr ice::EngineStateTrigger StateTrigger_DeactivateRuntime{
            .before = State_WorldInactive, // Parent state graph
            .from = State_WorldRuntimeActive,
            .to = State_WorldRuntimeInactive,
        };

        auto world_state_shards(void* userdata, ice::ShardContainer& out_shards) noexcept
        {
            ice::WorldStateParams const* params = reinterpret_cast<ice::WorldStateParams const*>(userdata);
            ice::Shard const shards[]{
                { ice::ShardID_WorldCreated },
                { ice::ShardID_WorldActivate | params },
                { ice::ShardID_WorldDeactivate | params },
                { ice::ShardID_WorldDestroyed },
                //{ ice::ShardID_RegisterDevUI | update_params.devui },
            };

            ice::shards::push_back(out_shards, shards);
        }

        auto devui_state_shards(void* userdata, ice::ShardContainer& out_shards) noexcept
        {
            //ice::WorldStateParams const* params = reinterpret_cast<ice::WorldStateParams const*>(userdata);
            //ice::Shard const shards[]{
            //    { ice::ShardID_RegisterDevUI | params->devui },
            //};

            //ice::shards::push_back(out_shards, shards);
        }

        //auto register_world_activation_flow(ice::WorldStateParams& params, ice::WorldStateTracker& world_states) noexcept -> ice::u8
        //{
        //    //ice::StackAllocator_1024 temp_alloc;
        //    //ice::Array<ice::WorldStateStage> stages{ temp_alloc };
        //    //ice::array::push_back(stages,
        //    //    ice::WorldStateStage{
        //    //        .trigger = ice::ShardID_WorldCreated,
        //    //        .resulting_state = 1
        //    //    }
        //    //);
        //    //ice::array::push_back(stages,
        //    //    ice::WorldStateStage{
        //    //        .trigger = ice::ShardID_WorldActivate,
        //    //        .required_state = 1,
        //    //        .resulting_state = 2,
        //    //        .notification = ice::ShardID_WorldActivated
        //    //    }
        //    //);
        //    //ice::array::push_back(stages,
        //    //    ice::WorldStateStage{
        //    //        .trigger = ice::ShardID_WorldDeactivate,
        //    //        .required_state = 2,
        //    //        .resulting_state = 1,
        //    //        .notification = ice::ShardID_WorldDeactivated
        //    //    }
        //    //);
        //    //ice::array::push_back(stages,
        //    //    ice::WorldStateStage{
        //    //        .trigger = ice::ShardID_WorldDestroyed,
        //    //        .required_state = 1,
        //    //        .resulting_state = 0
        //    //    }
        //    //);

        //    //ice::u8 const flowid_main = world_states.register_flow(
        //    //    ice::WorldStateFlow{
        //    //        .name = "world-activity"_sid,
        //    //        .stages = stages,
        //    //        .userdata = &params,
        //    //        .fn_shards = world_state_shards
        //    //    }
        //    //);

        //    //ice::array::clear(stages);
        //    //ice::array::push_back(stages,
        //    //    ice::WorldStateStage{
        //    //        .trigger = ice::ShardID_WorldCreated,
        //    //        .resulting_state = 1,
        //    //        .event_shard = ice::ShardID_RegisterDevUI
        //    //    }
        //    //);
        //    //world_states.register_flow(
        //    //    ice::WorldStateFlow{
        //    //        .name = "world-devui"_sid,
        //    //        .stages = stages,
        //    //        .userdata = &params,
        //    //        .fn_shards = devui_state_shards
        //    //    }
        //    //);

        //    //return flowid_main;
        //}

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
        ice::EngineRunnerCreateInfo const& create_info
    ) noexcept
        : _allocator{ alloc, "Engine Runner" }
        , _engine{ create_info.engine }
        , _clock{ create_info.clock }
        , _schedulers{ create_info.schedulers }
        , _frame_factory{ create_info.frame_factory }
        , _frame_factory_userdata{ create_info.frame_factory_userdata }
        , _frame_count{ create_info.concurrent_frame_count }
        , _frame_storage{ { _allocator }, { _allocator } }
        , _runtime_storage{ _allocator }
        , _frame_data_freelist{ nullptr }
        , _next_frame_index{ 0 }
        , _runner_tasks{ _allocator, _schedulers.tasks }
    {
        ICE_ASSERT(
            _frame_factory == _frame_factory_userdata || _frame_factory != nullptr,
            "It's UB to set factory userdata without a factory function!"
        );

        // Prepare frame data free list
        for (ice::u32 count = 0; count < _frame_count; ++count)
        {
            ice::IceshardFrameData* new_data = _allocator.create<ice::IceshardFrameData>(
                _allocator,
                _frame_storage[0],
                _runtime_storage,
                _runtime_storage
            );
            new_data->_internal_next = _frame_data_freelist.load(std::memory_order_relaxed);
            _frame_data_freelist.store(new_data, std::memory_order_relaxed);
        }

        ice::EngineStateTrigger triggers[]{ detail::StateTrigger_ActivateRuntime, detail::StateTrigger_DeactivateRuntime };
        _engine.states().register_graph(
            { .initial = State_WorldRuntimeInactive, .commiter = this, .enable_subname_states = true },
            triggers
        );
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
        _runner_tasks.wait_all();

        ice::u32 deleted_frame_data = 0;
        ice::IceshardFrameData* frame_data = _frame_data_freelist.load(std::memory_order_relaxed);
        while (frame_data != nullptr)
        {
            _allocator.destroy(std::exchange(frame_data, frame_data->_internal_next));
            deleted_frame_data += 1;
        }

        ICE_ASSERT(deleted_frame_data == _frame_count, "Failed to delete all frame data objects!");
    }

    auto IceshardEngineRunner::aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>>
    {
        IPT_ZONE_SCOPED;

        ice::UniquePtr<ice::EngineFrame> result;
        ice::IceshardFrameData* frame_data = _frame_data_freelist.load(std::memory_order_relaxed);
        if (frame_data != nullptr)
        {
            bool exchange_success;
            do
            {
                exchange_success = _frame_data_freelist.compare_exchange_weak(frame_data, frame_data->_internal_next, std::memory_order_relaxed);
            } while (exchange_success == false || frame_data == nullptr);

            if (frame_data != nullptr)
            {
                frame_data->_index = _next_frame_index.fetch_add(1, std::memory_order_relaxed);
                frame_data->_internal_next = frame_data; // Assing self (easy pointer access later and overflow check)
                result = _frame_factory(frame_data->_fwd_allocator, *frame_data, _frame_factory_userdata);
            }
        }

        co_return result;
    }

    auto IceshardEngineRunner::update_frame(ice::EngineFrame& current_frame, ice::EngineFrame const& previous_frame) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        ice::TaskContainer& current_tasks = current_frame.tasks_container();
        ice::EngineFrameUpdate const frame_update{
            .clock = _clock,
            .assets = _engine.assets(),
            .frame = current_frame,
            .last_frame = previous_frame,
            .thread = _schedulers,
        };

        ice::WorldUpdater& world_updater = _engine.worlds_updater();
        {
            EngineWorldUpdate const world_update{
                .clock = clock,
                .assets = _engine.assets(),
                .engine = _engine,
                .thread = _schedulers,
                .long_tasks = _runner_tasks
            };

            updater.update(frame, world_update, tasks);
        }

        ICE_ASSERT(
            ice::u8_max >= ice::array::count(tasks),
            "Gathered more tasks than it's possible to track!"
        );

        {
            IPT_ZONE_SCOPED_NAMED("world_tasks");
            co_await await_all_on(tasks, _schedulers.main); // Do we care about being resumed on the main thread?
            ice::array::clear(tasks);
        }

        {
            EngineFrameUpdate const frame_update{
                .clock = clock,
                .assets = _engine.assets(),
                //.runner = *this,
                .frame = frame,
                .last_frame = prev_frame,
                .thread = _schedulers,
                .long_tasks = _runner_tasks
            };

            IPT_ZONE_SCOPED_NAMED("gather_tasks");
            ice::Shard const update_shard[]{ ice::ShardID_FrameUpdate | &frame_update };
            world_updater.update(current_tasks, { update_shard});
            world_updater.update(current_tasks, { previous_frame.shards()._data });
        }

        // Execute all long tasks
        _runner_tasks.execute_all();

        co_await ice::await_all(tasks); // Don't care where we get resumed
        co_return;
    }

    void IceshardEngineRunner::release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept
    {
        IPT_ZONE_SCOPED;

        if (frame == nullptr)
        {
            return;
        }

        ice::IceshardFrameData* expected_head = _frame_data_freelist.load(std::memory_order_relaxed);
        ice::IceshardFrameData* const free_data = static_cast<ice::IceshardFrameData&>(frame->data())._internal_next;

        bool exchange_success;
        do
        {
            free_data->_internal_next = expected_head;
            exchange_success = _frame_data_freelist.compare_exchange_weak(expected_head, free_data, std::memory_order_relaxed);
        } while (exchange_success == false);
    }

    void IceshardEngineRunner::destroy() noexcept
    {
        _allocator.backing_allocator().destroy(this);
    }

    bool IceshardEngineRunner::commit(
        ice::EngineStateTrigger const& trigger,
        ice::Shard trigger_shard,
        ice::ShardContainer& out_shards
    ) noexcept
    {
        ice::WorldStateParams const params{
            .clock = _clock,
            .assets = _engine.assets(),
            .engine = _engine,
            .thread = _schedulers
        };

        ice::StringID_Hash world_name;
        if (ice::shard_inspect(trigger_shard, world_name) == false)
        {
            return false;
        }

        if (trigger.to == State_WorldRuntimeActive)
        {
            ice::wait_for(_engine.worlds().find_world({ world_name })->activate(params));
            ice::shards::push_back(out_shards, trigger.results | world_name);
        }
        else if (trigger.to == State_WorldRuntimeInactive)
        {
            ice::wait_for(_engine.worlds().find_world({ world_name })->deactivate(params));
        }
        return true;
    }

} // namespace ice

namespace ice
{

    auto create_engine_runner_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineRunnerCreateInfo const& create_info_arg
    ) noexcept -> ice::EngineRunner*
    {
        ice::EngineRunnerCreateInfo create_info = create_info_arg;

        if (create_info.concurrent_frame_count != 2)
        {
            ICE_LOG(LogSeverity::Error, LogTag::Engine, "Runner only allows two concurent frames to be aquired!");
            return nullptr;
        }

        if (create_info.frame_factory == nullptr)
        {
            create_info.frame_factory = ice::create_iceshard_frame;
        }

        return alloc.create<ice::IceshardEngineRunner>(alloc, create_info);
    }

    void destroy_engine_runner_fn(ice::EngineRunner* runner) noexcept
    {
        static_cast<ice::IceshardEngineRunner*>(runner)->destroy();
    }

} // namespace ice
