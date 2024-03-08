/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_world.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/engine_module.hxx>
#include <ice/engine_shards.hxx>
#include <ice/task_utils.hxx>
#include <ice/profiler.hxx>


namespace ice
{

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
        ice::EngineRunnerCreateInfo const& create_info
    ) noexcept
        : _allocator{ alloc }
        , _engine{ create_info.engine }
        , _schedulers{ .main = _main_scheduler, .tasks = create_info.schedulers.tasks }
        , _frame_factory{ create_info.frame_factory }
        , _frame_factory_userdata{ create_info.frame_factory_userdata }
        , _frame_count{ create_info.concurrent_frame_count }
        , _frame_storage{ { _allocator }, { _allocator } }
        , _runtime_storage{ _allocator }
        , _frame_data_freelist{ nullptr }
        , _next_frame_index{ 0 }
        , _main_queue{ }
        , _main_scheduler{ _main_queue }
        , _barrier{ }
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
                result = _frame_factory(_allocator, *frame_data, _frame_factory_userdata);
            }
        }

        co_return result;
    }

    auto IceshardEngineRunner::update_frame(
        ice::EngineFrame& frame,
        ice::EngineFrame const& prev_frame,
        ice::Clock const& clock
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;

        // We create once and always reset the object next time
        static ice::StackAllocator_2048 alloc{ };
        alloc.reset();
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex> tasks{ alloc };

        // TODO: Allow to set the number of tasks during world creation.
        ice::ucount constexpr max_capcity = ice::mem_max_capacity<ice::Task<>>(ice::StackAllocator_2048::Constant_InternalCapacity);
        ice::array::set_capacity(tasks, max_capcity);

        ice::WorldUpdater& updater = _engine.worlds_updater();
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

        _barrier.reset(ice::u8(ice::array::count(tasks)));
        {
            IPT_ZONE_SCOPED_NAMED("await_tasks");
            ice::manual_wait_for_all(tasks, _barrier);

            while (_barrier.is_set() == false)
            {
                // Process all tasks waiting in the queue
                for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_main_queue._awaitables))
                {
                    IPT_ZONE_SCOPED_NAMED("resume_awaitable");
                    awaitable->_coro.resume();
                }

                // TODO: Add a timed wait
                //_barrier.wait();
            }

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
            updater.update(ice::ShardID_FrameUpdate | &frame_update, tasks);
            updater.update(frame.shards(), tasks);
        }

        // Execute all long tasks
        _runner_tasks.execute_all();

        ICE_ASSERT(
            ice::u8_max >= ice::array::count(tasks),
            "Gathered more tasks than it's possible to track!"
        );

        _barrier.reset(ice::u8(ice::array::count(tasks)));
        {
            IPT_ZONE_SCOPED_NAMED("await_tasks");
            ice::manual_wait_for_all(tasks, _barrier);

            while (_barrier.is_set() == false)
            {
                // Process all tasks waiting in the queue
                for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_main_queue._awaitables))
                {
                    IPT_ZONE_SCOPED_NAMED("resume_awaitable");
                    awaitable->_coro.resume();
                }

                // TODO: Add a timed wait
                //_barrier.wait();
            }
        }

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
        _allocator.destroy(this);
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
