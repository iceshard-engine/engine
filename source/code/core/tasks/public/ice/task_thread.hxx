#pragma once
#include <ice/unique_ptr.hxx>

namespace ice
{

    template<typename T>
    class Task;

    class TaskScheduler
    {
    public:
        virtual ~TaskScheduler() noexcept = default;

        virtual void schedule(ice::Task<void> task) noexcept = 0;
    };

    class TaskThread
    {
    public:
        virtual ~TaskThread() noexcept = default;

        virtual void stop() noexcept = 0;

        virtual void join() noexcept = 0;

        virtual auto scheduler() noexcept -> ice::TaskScheduler& = 0;
    };

    auto create_task_thread(ice::Allocator& alloc) noexcept -> ice::UniquePtr<TaskThread>;

} // namespace ice
