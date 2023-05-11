/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_frame.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>

#include <ice/input/input_event.hxx>
#include <ice/task.hxx>

#include <ice/mem_allocator_ring.hxx>
#include <ice/container_types.hxx>
#include <ice/container/array.hxx>
#include <atomic>

#include "iceshard_task_executor.hxx"

namespace ice
{

    class IceshardMemoryFrame final : public ice::EngineFrame
    {
    public:
        IceshardMemoryFrame(
            ice::RingAllocator& alloc
        ) noexcept;
        ~IceshardMemoryFrame() noexcept override;

        auto index() const noexcept -> ice::u32 override;

        auto allocator() noexcept -> ice::Allocator& override;

        auto input_events() noexcept -> ice::Array<ice::input::InputEvent>&;
        auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> override;

        void execute_task(ice::Task<void> task) noexcept;
        void start_all() noexcept;
        void wait_ready() noexcept;

        auto shards() noexcept -> ice::ShardContainer& override;
        auto shards() const noexcept -> ice::ShardContainer const& override;

        auto entity_operations() noexcept -> ice::ecs::EntityOperations& override;
        auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& override;

        auto storage() noexcept -> ice::DataStorage& override;
        auto storage() const noexcept -> ice::DataStorage const& override;

        auto stage_end() noexcept -> ice::TaskStage<ice::EngineFrame> override;

    private:
        ice::u32 const _index;
        ice::RingAllocator& _allocator;
        ice::RingAllocator _inputs_allocator;
        ice::RingAllocator _shards_allocator;
        ice::RingAllocator _tasks_allocator;
        ice::RingAllocator _storage_allocator;
        ice::RingAllocator _data_allocator;

        ice::Array<ice::input::InputEvent> _input_events;
        ice::ShardContainer _shards;
        ice::ecs::EntityOperations _entity_operations;
        ice::HashedDataStorage _data_storage;

        ice::Array<ice::Task<>, ice::ContainerLogic::Complex> _frame_tasks;
        ice::IceshardTaskExecutor _task_executor;

        ice::TaskQueue _queue_frame_end;
    };

} // namespace ice
