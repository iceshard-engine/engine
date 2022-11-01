/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
//#include <ice/task_scheduler.hxx>
#include <ice/task_operations.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/task.hxx>

namespace ice
{

    class TaskThreadPool
    {
    public:
        virtual ~TaskThreadPool() noexcept = default;

        using ScheduleOperation = ice::ScheduleOperation<ice::TaskThreadPool>;

        virtual void schedule_internal(
            ScheduleOperation* op,
            ScheduleOperation::DataMemberType data_member
        ) noexcept = 0;

        inline auto schedule() noexcept -> ScheduleOperation;

        inline auto operator co_await() noexcept { return schedule(); }
    };

    inline auto TaskThreadPool::schedule() noexcept -> ScheduleOperation
    {
        return ScheduleOperation{ *this };
    }

    auto create_simple_threadpool(
        ice::Allocator& alloc,
        ice::u32 thread_count
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool>;

} // namespace ice
