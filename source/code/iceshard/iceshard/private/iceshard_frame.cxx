/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_frame.hxx"
#include "iceshard_task_executor.hxx"
#include <ice/engine_shards.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::usize InputsAllocatorCapacity = ice::size_of<ice::input::InputEvent> * 512;
        static constexpr ice::usize RequestAllocatorCapacity = ice::size_of<ice::Shard> * 256;
        static constexpr ice::usize TaskAllocatorCapacity = ice::size_of<ice::Task<>> * 1024;
        static constexpr ice::usize StorageAllocatorCapacity = 8_MiB;
        static constexpr ice::usize DataAllocatorCapacity = 8_MiB;

        static ice::u32 global_frame_counter = 0;

    } // namespace detail

    IceshardMemoryFrame::IceshardMemoryFrame(
        ice::RingAllocator& alloc
    ) noexcept
        : ice::EngineFrame{ }
        , _index{ detail::global_frame_counter }
        , _allocator{ alloc }
        , _inputs_allocator{ _allocator, "inputs", { detail::InputsAllocatorCapacity } }
        , _shards_allocator{ _allocator, "shards", { detail::RequestAllocatorCapacity } }
        , _tasks_allocator{ _allocator, "tasks", { detail::TaskAllocatorCapacity } }
        , _storage_allocator{ _allocator, "storage", { detail::StorageAllocatorCapacity } }
        , _data_allocator{ _allocator, "data", { detail::DataAllocatorCapacity } }
        , _input_events{ _inputs_allocator }
        , _shards{ _shards_allocator }
        , _entity_operations{ _data_allocator } // #todo change the allocator?
        , _data_storage{ _storage_allocator }
        , _frame_tasks{ _tasks_allocator }
        , _task_executor{ _allocator, IceshardTaskExecutor::TaskList{ _allocator } }
    {
        detail::global_frame_counter += 1;

        ice::array::reserve(_input_events, ice::mem_max_capacity<ice::input::InputEvent>(detail::InputsAllocatorCapacity) - 4);
        ice::shards::reserve(_shards, ice::mem_max_capacity<ice::Shard>(detail::RequestAllocatorCapacity) - 4);
        ice::array::reserve(_frame_tasks, ice::mem_max_capacity<ice::Task<>>(detail::TaskAllocatorCapacity) - 4);

        ice::shards::push_back(_shards, ice::Shard_FrameTick | _index);
    }

    IceshardMemoryFrame::~IceshardMemoryFrame() noexcept
    {
        ice::FrameEndOperationData* operation_head = _frame_end_operation;
        while (_frame_end_operation.compare_exchange_weak(operation_head, nullptr, std::memory_order::relaxed, std::memory_order::acquire) == false)
        {
            continue;
        }

        ice::EngineTaskOperationBaseData* operation = operation_head;
        while (operation != nullptr)
        {
            ice::EngineTaskOperationBaseData* next_operation = operation->next;
            operation->coroutine.resume();
            operation = next_operation;
        }

        _task_executor.wait_ready();
    }

    auto IceshardMemoryFrame::index() const noexcept -> ice::u32
    {
        return _index;
    }

    auto IceshardMemoryFrame::allocator() noexcept -> ice::Allocator&
    {
        return _data_allocator;
    }

    auto IceshardMemoryFrame::input_events() noexcept -> ice::Array<ice::input::InputEvent>&
    {
        return _input_events;
    }

    auto IceshardMemoryFrame::input_events() const noexcept -> ice::Span<ice::input::InputEvent const>
    {
        return _input_events;
    }

    void IceshardMemoryFrame::execute_task(ice::Task<void> task) noexcept
    {
        // Adds a task to be executed later during the frame update.
        ice::array::push_back(_frame_tasks, ice::move(task));
    }

    void IceshardMemoryFrame::start_all() noexcept
    {
        _task_executor = IceshardTaskExecutor{ _allocator, ice::move(_frame_tasks) };
        _task_executor.start_all();
    }

    void IceshardMemoryFrame::wait_ready() noexcept
    {
        _task_executor.wait_ready();
    }

    auto IceshardMemoryFrame::shards() noexcept -> ice::ShardContainer&
    {
        return _shards;
    }

    auto IceshardMemoryFrame::shards() const noexcept -> ice::ShardContainer const&
    {
        return _shards;
    }

    auto IceshardMemoryFrame::entity_operations() noexcept -> ice::ecs::EntityOperations&
    {
        return _entity_operations;
    }

    auto IceshardMemoryFrame::entity_operations() const noexcept -> ice::ecs::EntityOperations const&
    {
        return _entity_operations;
    }

    auto IceshardMemoryFrame::storage() noexcept -> ice::DataStorage&
    {
        return _data_storage;
    }

    auto IceshardMemoryFrame::storage() const noexcept -> ice::DataStorage const&
    {
        return _data_storage;
    }

    auto IceshardMemoryFrame::schedule_frame_end() noexcept -> ice::FrameEndOperation
    {
        return { *this };
    }

    void IceshardMemoryFrame::schedule_internal(ice::FrameEndOperationData& operation) noexcept
    {
        ice::FrameEndOperationData* expected_head = _frame_end_operation.load(std::memory_order_acquire);

        do
        {
            operation.next = expected_head;
        }
        while (
            _frame_end_operation.compare_exchange_weak(
                expected_head,
                &operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceshardMemoryFrame::schedule_query_internal(ice::ecs::ScheduledQueryData& query_data) noexcept
    {
        ice::ecs::ScheduledQueryData* expected_head = _query_operation.load(std::memory_order_acquire);

        do
        {
            query_data.next = expected_head;
        } while (
            _query_operation.compare_exchange_weak(
                expected_head,
                &query_data,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

} // namespace ice
