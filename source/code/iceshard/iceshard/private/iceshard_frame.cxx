/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_frame.hxx"
#include <ice/task_utils.hxx>
#include <ice/engine.hxx>

namespace ice
{

    IceshardEngineFrame::IceshardEngineFrame(ice::IceshardFrameData& frame_data) noexcept
        : _frame_data{ frame_data }
        , _data{ _frame_data._fwd_allocator }
        , _shards{ _frame_data._fwd_allocator }
        , _operations{
            _frame_data._fwd_allocator,
            frame_data._engine.entity_index(),
            frame_data._engine.entity_archetypes(),
            16
        }
        , _task_groups{ _frame_data._fwd_allocator }
    {
        _task_groups.reserve(32);
    }

    IceshardEngineFrame::~IceshardEngineFrame() noexcept
    {
        for (TaskGroup& group : _task_groups)
        {
            ICE_ASSERT_CORE(group.barrier->is_set() == true);
            _frame_data._fwd_allocator.deallocate(group.barrier);
        }
        _task_groups.clear();
        ice::hashmap::clear(_data._values);

        _frame_data._fwd_allocator.reset();
    }

    auto IceshardEngineFrame::entity_index() noexcept -> ice::ecs::EntityIndex&
    {
        return _frame_data._engine.entity_index();
    }

    auto IceshardEngineFrame::create_tasks(ice::u32 count, ice::ShardID id) noexcept -> ice::Span<ice::Task<>>
    {
        if (count == 0) return { };

        _task_groups.push_back(
            TaskGroup{
                .tasks = ice::Array<ice::Task<>>{ _frame_data._fwd_allocator },
                .barrier = _frame_data._fwd_allocator.create<ice::ManualResetBarrier>()
            }
        );
        ice::Array<ice::Task<>>& result = _task_groups.last().tasks;
        result.resize(count);
        return result;
    }

    auto IceshardEngineFrame::await_tasks_scheduled_on(ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        ice::u32 task_count = 0;
        for (TaskGroup& group : _task_groups)
        {
            task_count += group.tasks.size().u32();
        }

        if (task_count > 0)
        {
            ice::Array<ice::Task<>> final_task_list{ _frame_data._fwd_allocator };
            final_task_list.reserve(task_count);

            for (TaskGroup& group : _task_groups)
            {
                for (ice::Task<>& task : group.tasks)
                {
                    final_task_list.push_back(ice::move(task));
                }
                group.tasks.clear();
            }

            co_await ice::await_scheduled_on(final_task_list, scheduler, resumer);
        }
    }

    auto IceshardEngineFrame::execute_tasks() noexcept -> ice::u32
    {
        ice::u32 total_count = 0;
        for (TaskGroup& group : _task_groups)
        {
            ice::u32 const current_count = group.tasks.size().u32();

            // Only reset the barrier if we actual have tasks to execute.
            if (current_count > 0)
            {
                total_count += current_count;

                ICE_ASSERT_CORE(group.tasks.size() < ice::u8_max);

                group.barrier->reset(group.tasks.size().u8());
                ice::manual_wait_for(*group.barrier, group.tasks);
                group.tasks.clear();
            }
        }
        return total_count;
    }

    auto IceshardEngineFrame::running_tasks() const noexcept -> ice::u32
    {
        ice::u32 result = 0;
        for (TaskGroup const& group : _task_groups)
        {
            result += ice::u32(group.barrier->is_set() == false);
        }
        return result;
    }

    void IceshardEngineFrame::wait_tasks() noexcept
    {
        for (TaskGroup& group : _task_groups)
        {
            group.barrier->wait();
        }
    }

    auto IceshardEngineFrame::extract_tasks() noexcept -> ice::Array<ice::Task<>>
    {
        return  ice::Array<ice::Task<>>{ *_shards._data._allocator };
    }

    auto create_iceshard_frame(
        ice::Allocator& alloc,
        ice::EngineFrameData& frame_data,
        ice::EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>
    {
        return ice::make_unique<ice::IceshardEngineFrame>(alloc, static_cast<ice::IceshardFrameData&>(frame_data));
    }

} // namespace ice
