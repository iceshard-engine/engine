#include <ice/task_utils_v2.hxx>
#include "internal_tasks/task_utils.hxx"

namespace ice::v2
{

    auto await_on(ice::Task<> task, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await task;
        co_await resumer;
    }

    auto await_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_tasks(tasks);
        co_await resumer;
    }

    auto schedule(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        ice::schedule_detached_task(ice::move(task), scheduler);
        co_return;
    }

    auto schedule(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        for (ice::Task<>& task : tasks)
        {
            ice::schedule_detached_task(ice::move(task), scheduler);
        }
        co_return;
    }

    auto schedule_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        ice::TaskQueue& scheduler_queue = scheduler.schedule()._queue;

        // Move the queue task to the given scheduler.
        scheduler_queue.push_back(queue.consume());
        co_return;
    }

    auto schedule_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        ice::TaskQueue& scheduler_queue = scheduler.schedule()._queue;

        // We set the result value for each awaitable in the queue and nothing more.
        ice::LinkedQueueRange<ice::TaskAwaitableBase> tasks_awaitables = queue.consume();
        for (ice::TaskAwaitableBase* task_awaitable : tasks_awaitables)
        {
            ICE_ASSERT_CORE(task_awaitable->result.ptr == nullptr);
            task_awaitable->result.ptr = result;
        }

        // Move the queue task to the given scheduler.
        scheduler_queue.push_back(tasks_awaitables);
        co_return;
    }

    auto await_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        co_await scheduler;
        co_await task;
    }

    auto await_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_tasks(tasks, scheduler);
    }

    auto await_scheduled_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_queue(queue, nullptr, scheduler);
    }

    auto await_scheduled_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_queue(queue, result, scheduler);
    }

    auto await_scheduled_on(ice::Task<> task, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await await_scheduled(ice::move(task), scheduler);
        co_await resumer;
    }

    auto await_scheduled_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_tasks(tasks, scheduler);
        co_await resumer;
    }

    auto await_scheduled_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_queue(queue, nullptr, scheduler);
        co_await resumer;
    }

    auto await_scheduled_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_queue(queue, result, scheduler);
        co_await resumer;
    }

    void manual_wait_for(ice::ManualResetEvent& event, ice::Task<> task) noexcept
    {
        execute_detached_task(ice::move(task), event);
    }

    void manual_wait_for(ice::ManualResetBarrier& barrier, ice::Span<ice::Task<>> tasks) noexcept
    {
        for (ice::Task<>& task : tasks)
        {
            execute_detached_task(ice::move(task), barrier);
        }
    }

    void manual_wait_for_scheduled(ice::ManualResetEvent& event, ice::Task<> task, ice::TaskScheduler& scheduler) noexcept
    {
        schedule_detached_task(ice::move(task), scheduler, event);
    }

    void manual_wait_for_scheduled(ice::ManualResetBarrier& barrier, ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        for (ice::Task<>& task : tasks)
        {
            schedule_detached_task(ice::move(task), scheduler, barrier);
        }
    }

    void wait_for(ice::Task<> task) noexcept
    {
        ice::ManualResetEvent ev{};
        manual_wait_for(ev, ice::move(task));
        ev.wait();
    }

    void wait_for(ice::Span<ice::Task<>> tasks) noexcept
    {
        ice::ManualResetBarrier ev{};
        manual_wait_for(ev, tasks);
        ev.wait();
    }

    void wait_for_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept
    {
        ice::ManualResetEvent ev{};
        manual_wait_for_scheduled(ev, ice::move(task), scheduler);
        ev.wait();
    }

    void wait_for_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        ice::ManualResetBarrier ev{};
        manual_wait_for_scheduled(ev, tasks, scheduler);
        ev.wait();
    }

} // namespace ice::v2
