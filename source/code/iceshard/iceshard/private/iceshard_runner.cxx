/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_world.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/engine_state_tracker.hxx>
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

    }

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
        for (ice::u32 frame_idx = 0; frame_idx < _frame_count; ++frame_idx)
        {
            ice::IceshardFrameData* new_data = _allocator.create<ice::IceshardFrameData>(
                _allocator,
                _engine,
                _frame_storage[frame_idx],
                _runtime_storage,
                _runtime_storage
            );
            new_data->_internal_next = _frame_data_freelist.load(std::memory_order_relaxed);
            _frame_data_freelist.store(new_data, std::memory_order_relaxed);
        }

        ice::EngineStateTrigger triggers[]{ detail::StateTrigger_ActivateRuntime, detail::StateTrigger_DeactivateRuntime };
        _engine.states().register_graph(
            { .initial = State_WorldRuntimeInactive, .committer = this, .enable_subname_states = true },
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
                ice::hashmap::clear(frame_data->_storage_frame._values);
                result = _frame_factory(frame_data->_fwd_allocator, *frame_data, _frame_factory_userdata);
            }
        }

        co_return result;
    }

    auto IceshardEngineRunner::update_frame(
        ice::EngineFrame& frame,
        ice::EngineFrame const& previous_frame
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        ice::TaskContainer& current_tasks = frame.tasks_container();
        ice::EngineFrameUpdate const frame_update{
            .clock = _clock,
            .assets = _engine.assets(),
            .frame = frame,
            .last_frame = previous_frame,
            .thread = _schedulers,
        };

        ice::WorldUpdater& world_updater = _engine.worlds_updater();
        {
            IPT_ZONE_SCOPED_NAMED("gather_tasks");
            ice::Shard const update_shard[]{ ice::ShardID_FrameUpdate | &frame_update };
            world_updater.update(current_tasks, { update_shard });
            world_updater.update(current_tasks, previous_frame.shards());
        }

        // Execute all frame tasks
        current_tasks.execute_tasks();
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

        // Delete the frame explicitly
        // frame.reset();
    }

    auto IceshardEngineRunner::apply_entity_operations(
        ice::ShardContainer& out_shards
    ) noexcept -> ice::Task<>
    {
        _engine.worlds_updater().apply_entity_operations(out_shards);
        co_return;
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
        ice::StringID world_name;
        if (ice::shard_inspect(trigger_shard, world_name.value) == false)
        {
            return false;
        }

        ice::World* world = _engine.worlds().find_world(world_name);
        ICE_ASSERT_CORE(world != nullptr);

        ice::WorldStateParams const params{
            .clock = _clock,
            .assets = _engine.assets(),
            .engine = _engine,
            .thread = _schedulers,
            .world = *world
        };

        if (trigger.to == State_WorldRuntimeActive)
        {
            ice::v2::wait_for(world->activate(params));
            ice::shards::push_back(out_shards, trigger.results | world_name.value);
        }
        else if (trigger.to == State_WorldRuntimeInactive)
        {
            ice::v2::wait_for(world->deactivate(params));
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
