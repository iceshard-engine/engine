#pragma once
//#include <ice/task_scheduler.hxx>
#include <ice/task_operations.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/allocator.hxx>
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
