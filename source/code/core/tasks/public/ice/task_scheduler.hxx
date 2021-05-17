#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename T>
    class Task;

    class SimpleTaskThread;
    class ScheduleOperation;

    class TaskScheduler
    {
    public:
        virtual ~TaskScheduler() noexcept = default;

        virtual void schedule(ice::Task<void> task) noexcept = 0;

        virtual auto schedule_test() noexcept -> ice::ScheduleOperation = 0;
    };

    class ScheduleOperation
    {
    public:
        ScheduleOperation(ice::TaskScheduler& scheduker) noexcept;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<void> coro) noexcept;
        void await_resume() const noexcept { }

    private:
        friend class SimpleTaskThread;

        ice::TaskScheduler& _scheduler;
        std::coroutine_handle<> _coro;
        ice::ScheduleOperation* _next;
    };

} // namespace ice
