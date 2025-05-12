/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_forward.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/container/hashmap.hxx>

#include "iceshard_data_storage.hxx"

namespace ice
{

    static constexpr ice::EngineStateGraph StateGraph_WorldRuntime = "world-runtime"_state_graph;
    static constexpr ice::EngineState State_WorldRuntimeActive = StateGraph_WorldRuntime | "active";
    static constexpr ice::EngineState State_WorldRuntimeInactive = StateGraph_WorldRuntime | "inactive";

    static constexpr ice::ShardID ShardID_WorldRuntimeActivated = "event/world-runtime/activated`ice::StringID_Hash"_shardid;

    class IceshardEngine;

    struct IceshardFrameData : ice::EngineFrameData
    {
        ice::ProxyAllocator _allocator;
        ice::ForwardAllocator _fwd_allocator;

        ice::Engine& _engine;

        ice::IceshardDataStorage& _storage_runtime;
        ice::IceshardDataStorage const& _storage_persistent;

        ice::u32 _index;

        // Only used for internal purposes
        ice::IceshardFrameData* _internal_next;

        IceshardFrameData(
            ice::Allocator& alloc,
            ice::Engine& engine,
            ice::IceshardDataStorage& runtime_storage,
            ice::IceshardDataStorage& persistent_storage
        ) noexcept
            : _allocator{ alloc, "frame" }
            , _fwd_allocator{ _allocator, "frame-forward", ForwardAllocatorParams{.bucket_size = 16_KiB, .min_bucket_count = 2} }
            , _engine{ engine }
            , _storage_runtime{ runtime_storage }
            , _storage_persistent{ persistent_storage }
            , _index{ }
            , _internal_next{ nullptr }
        { }

        virtual ~IceshardFrameData() noexcept override = default;

        auto runtime() noexcept -> ice::DataStorage& override
        {
            return _storage_runtime;
        }

        auto runtime() const noexcept -> ice::DataStorage const& override
        {
            return _storage_runtime;
        }

        auto persistent() const noexcept -> ice::DataStorage const& override
        {
            return _storage_persistent;
        }
    };

    class IceshardEngineTaskContainer final : public ice::EngineTaskContainer
    {
    public:
        IceshardEngineTaskContainer(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler
        ) noexcept
            : _scheduler{ scheduler }
            , _running_tasks{ 0 }
            , _pending_tasks{ alloc }
        {
        }

        ~IceshardEngineTaskContainer() noexcept
        {
            bool const has_running_tasks = _running_tasks.load(std::memory_order_relaxed) > 0;
            // while(_running_tasks.load(std::memory_order_relaxed) > 0)
            {
                // TODO: Sleep for 500ms to allow tasks to finish?
                // TODO: Add cancelation tokens
            }

            ICE_ASSERT_CORE(has_running_tasks == false);
        }

        void execute(ice::Task<> task) noexcept override
        {
            ice::array::push_back(_pending_tasks, ice::move(task));
        }

        auto execute_internal(ice::Task<> task) noexcept -> ice::Task<>
        {
            co_await task;
            _running_tasks.fetch_sub(1, std::memory_order_relaxed);
        }

        void execute_all() noexcept
        {
            ice::ucount const num_tasks = ice::array::count(_pending_tasks);
            _running_tasks.fetch_add(num_tasks, std::memory_order_relaxed);

            for (ice::Task<>& pending_task : _pending_tasks)
            {
                // We schedule the task on the given scheduler.
                ice::schedule_task(execute_internal(ice::move(pending_task)), _scheduler);
            }
            ice::array::clear(_pending_tasks);
        }

        void wait_all() noexcept
        {
            bool const has_running_tasks = _running_tasks.load(std::memory_order_relaxed) > 0;
            // while(_running_tasks.load(std::memory_order_relaxed) > 0)
            {
                // TODO: Sleep for 500ms to allow tasks to finish?
                // TODO: Add cancelation tokens
            }

            ICE_ASSERT_CORE(has_running_tasks == false);
        }

    private:
        ice::TaskScheduler& _scheduler;
        std::atomic_uint32_t _running_tasks;
        ice::Array<ice::Task<>> _pending_tasks;
    };

    class IceshardEngineRunner
        : public ice::EngineRunner
        , public ice::EngineStateCommitter
    {
    public:
        ~IceshardEngineRunner() noexcept override;
        IceshardEngineRunner(ice::Allocator& alloc, ice::EngineRunnerCreateInfo const& create_info) noexcept;

        auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> override;
        auto update_frame(ice::EngineFrame& current_frame, ice::EngineFrame const& previous_frame) noexcept -> ice::Task<> override;
        void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept override;

        auto pre_update(ice::ShardContainer& out_shards) noexcept -> ice::Task<> override;

        void destroy() noexcept;

    public: // Impl: ice::EngineStateCommiter
        bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::Engine& _engine;
        ice::Clock const& _clock;
        ice::EngineSchedulers _schedulers;
        ice::EngineFrameFactory const _frame_factory;
        ice::EngineFrameFactoryUserdata const _frame_factory_userdata;
        ice::u32 const _frame_count;

        ice::IceshardDataStorage _runtime_storage;

        std::atomic<ice::IceshardFrameData*> _frame_data_freelist;
        std::atomic<ice::u32> _next_frame_index;

        ice::IceshardEngineTaskContainer _runner_tasks;
    };

} // namespace ice
