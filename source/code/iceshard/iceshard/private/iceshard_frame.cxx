/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_frame.hxx"
#include <ice/task_utils.hxx>
#include <ice/engine.hxx>

namespace ice
{

    IceshardEngineFrame::IceshardEngineFrame(ice::IceshardFrameData& frame_data) noexcept
        : _data{ frame_data }
        , _shards{ _data._fwd_allocator }
        , _operations{ frame_data._fwd_allocator, 16 }
        , _task_groups{ _data._fwd_allocator }
    {
        ice::array::reserve(_task_groups, 32);
    }

    IceshardEngineFrame::~IceshardEngineFrame() noexcept
    {
        for (TaskGroup& group : _task_groups)
        {
            ICE_ASSERT_CORE(group.barrier->is_set() == true);
            _data._fwd_allocator.deallocate(group.barrier);
        }
        ice::array::clear(_task_groups);

        _data._fwd_allocator.reset();
    }

    auto IceshardEngineFrame::entity_index() noexcept -> ice::ecs::EntityIndex&
    {
        return _data._engine.entities();
    }

    auto IceshardEngineFrame::create_tasks(ice::u32 count, ice::ShardID id) noexcept -> ice::Span<ice::Task<>>
    {
        if (count == 0) return { };

        ice::array::push_back(_task_groups,
            TaskGroup{
                .tasks = ice::Array<ice::Task<>>{ _data._fwd_allocator },
                .barrier = _data._fwd_allocator.create<ice::ManualResetBarrier>()
            }
        );
        ice::Array<ice::Task<>>& result = ice::array::back(_task_groups).tasks;
        ice::array::resize(result, count);
        return result;
    }

    auto IceshardEngineFrame::execute_tasks() noexcept -> ice::ucount
    {
        ice::ucount total_count = 0;
        for (TaskGroup& group : _task_groups)
        {
            total_count += ice::count(group.tasks);

            // Only reset the barrier if we actual have tasks to execute.
            if (total_count > 0)
            {
                ICE_ASSERT_CORE(ice::count(group.tasks) < ice::u8_max);

                group.barrier->reset(ice::u8(ice::count(group.tasks)));
                ice::manual_wait_for_all(group.tasks, *group.barrier);
                ice::array::clear(group.tasks);
            }
        }
        return total_count;
    }

    auto IceshardEngineFrame::running_tasks() const noexcept -> ice::ucount
    {
        ice::ucount result = 0;
        for (TaskGroup const& group : _task_groups)
        {
            result += ice::ucount(group.barrier->is_set() == false);
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

    auto create_iceshard_frame(
        ice::Allocator& alloc,
        ice::EngineFrameData& frame_data,
        ice::EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>
    {
        return ice::make_unique<ice::IceshardEngineFrame>(alloc, static_cast<ice::IceshardFrameData&>(frame_data));
    }

} // namespace ice
