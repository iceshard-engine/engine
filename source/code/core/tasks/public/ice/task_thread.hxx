#pragma once
#include <ice/task.hxx>
#include <ice/task_operations.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class TaskThread
    {
    public:

        virtual ~TaskThread() noexcept = default;

        virtual void stop() noexcept = 0;

        virtual void join() noexcept = 0;

        [[deprecated("This API method is obsolete and should no longer be used!")]]
        virtual void schedule(ice::Task<void> task) noexcept = 0;

        using ScheduleOperation = ice::ScheduleOperation<ice::TaskThread>;

        virtual void schedule_internal(
            ScheduleOperation* op,
            ScheduleOperation::DataMemberType data_member
        ) noexcept = 0;
    };

    class TaskThread_v2 : public ice::TaskScheduler_v2
    {
    public:
        virtual ~TaskThread_v2() noexcept = default;

        virtual void stop() noexcept = 0;

        virtual void join() noexcept = 0;
    };

    auto create_task_thread(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::TaskThread>;
    auto create_task_thread_v2(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::TaskThread_v2>;

} // namespace ice
