/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_frame.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/task_container.hxx>
#include "iceshard_runner.hxx"
#include "iceshard_world.hxx"

namespace ice
{

    struct IceshardEngineFrame : EngineFrame, TaskContainer
    {
        IceshardEngineFrame(
            ice::IceshardFrameData& frame_data
        ) noexcept;
        ~IceshardEngineFrame() noexcept override;

        auto allocator() const noexcept -> ice::Allocator& override { return _frame_data._allocator; }
        auto index() const noexcept -> ice::u32 override { return _frame_data._index; }

        auto data() noexcept -> ice::DataStorage& override { return _data; }
        auto data() const noexcept -> ice::DataStorage const& override { return _data; }
        auto frame_data() noexcept -> ice::EngineFrameData& { return _frame_data; }
        auto frame_data() const noexcept -> ice::EngineFrameData const& { return _frame_data; }

        auto shards() noexcept -> ice::ShardContainer& override { return _shards; }
        auto shards() const noexcept -> ice::ShardContainer const& override { return _shards; }

        auto entity_index() noexcept -> ice::ecs::EntityIndex& override;
        auto entity_operations() noexcept -> ice::ecs::EntityOperations& override { return _operations; }
        auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& override { return _operations; }

        auto tasks_container() noexcept -> ice::TaskContainer& override { return *this; }

        auto create_tasks(ice::u32 count, ice::ShardID id) noexcept -> ice::Span<ice::Task<>> override;
        auto await_tasks_scheduled_on(ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<> override;
        auto execute_tasks() noexcept -> ice::ucount override;
        auto running_tasks() const noexcept -> ice::ucount override;
        void wait_tasks() noexcept override;

        auto extract_tasks() noexcept -> ice::Array<ice::Task<>> override;

    private:
        ice::IceshardFrameData& _frame_data;
        ice::IceshardDataStorage _data;
        ice::ShardContainer _shards;
        ice::ecs::EntityOperations _operations;

        struct TaskGroup
        {
            ice::Array<ice::Task<>> tasks;
            ice::ManualResetBarrier* barrier;
        };

        ice::Array<TaskGroup> _task_groups;
    };

    auto create_iceshard_frame(
        ice::Allocator& alloc,
        ice::EngineFrameData& frame_data,
        ice::EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>;

} // namespace ice
