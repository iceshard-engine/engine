#pragma once
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sync_manual_events.hxx>
#include "task_tracked.hxx"
#include "task_detached.hxx"

namespace ice
{

    template<typename T>
    concept HasSetMethod = requires(T t) {
        { t.set() } -> std::convertible_to<void>;
    };

    inline auto execute_detached_task(
        ice::Task<> task
    ) noexcept -> ice::DetachedTask
    {
        co_await task;
    }

    inline auto execute_detached_task(
        ice::Task<> task,
        ice::HasSetMethod auto& event
    ) noexcept -> ice::DetachedTask
    {
        co_await task;
        event.set();
    }

    inline auto schedule_detached_task(
        ice::Task<> scheduled_task,
        ice::TaskScheduler& scheduler
    ) noexcept -> ice::DetachedTask
    {
        co_await scheduler;
        co_await scheduled_task;
    }

    inline auto schedule_detached_task(
        ice::Task<> scheduled_task,
        ice::TaskScheduler& scheduler,
        ice::HasSetMethod auto& event
    ) noexcept -> ice::DetachedTask
    {
        co_await scheduler;
        co_await scheduled_task;
        event.set();
    }

    // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
    inline auto execute_tracked_task(
        ice::Task<void>& awaited_task,
        std::coroutine_handle<>,
        std::atomic_uint32_t&
    ) noexcept -> ice::TrackedTask
    {
        co_await awaited_task;
    }

#if 0
    // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
    inline auto execute_tracked_queue(
        ice::TaskQueue&,
        std::atomic_uint32_t&
    ) noexcept -> ice::TrackedQueue
    {
        co_return;
    }
#endif

    // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
    inline auto schedule_tracked_task(
        ice::Task<void>& awaited_task,
        std::coroutine_handle<>,
        std::atomic_uint32_t&,
        ice::TaskScheduler& scheduler
    ) noexcept -> ice::TrackedTask
    {
        co_await scheduler;
        co_await awaited_task;
    }

#if 0
    // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
    inline auto schedule_tracked_queue(
        ice::TaskQueue&,
        std::atomic_uint32_t&,
        ice::TaskScheduler& scheduler
    ) noexcept -> ice::TrackedQueue
    {
        // We await the scheduler, so when we enter 'final_awaitable'
        //  it will continue a task from the queue on that scheduled thread.
        co_await scheduler;
        co_return;
    }
#endif

    inline auto internal_await_tasks(ice::Span<ice::Task<>> tasks) noexcept
    {
        struct Awaitable
        {
            ice::Span<ice::Task<void>> tasks;
            std::atomic_uint32_t running;

            constexpr auto await_ready() const noexcept
            {
                // Only suspend if we actually have tasks
                return ice::span::empty(tasks);
            }

            inline auto await_suspend(std::coroutine_handle<> coro) noexcept
            {
                // Set the 'running' variable so we can track how many tasks arleady finished.
                running.store(ice::count(tasks) + 1, std::memory_order_relaxed);

                for (ice::Task<void>& task : tasks)
                {
                    // Execute all tasks
                    ice::execute_tracked_task(task, coro, running);
                }

                // We now return from the coroutine if the tasks finished synchronously.
                return running.fetch_sub(1, std::memory_order_relaxed) != 1;
            }

            constexpr bool await_resume() const noexcept
            {
                ICE_ASSERT_CORE(running.load(std::memory_order_relaxed) == 0);
                return ice::span::any(tasks);
            }
        };
        return Awaitable{ tasks };
    }

    inline auto internal_await_scheduled_tasks(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        struct Awaitable
        {
            ice::Span<ice::Task<>> tasks;
            ice::TaskScheduler& scheduler;
            std::atomic_uint32_t running;

            constexpr auto await_ready() const noexcept
            {
                // Only suspend if we actually have tasks
                return ice::span::empty(tasks);
            }

            inline auto await_suspend(std::coroutine_handle<> coro) noexcept
            {
                // Set the 'running' variable so we can track how many tasks arleady finished.
                running.store(ice::count(tasks) + 1, std::memory_order_relaxed);

                for (ice::Task<>& task : tasks)
                {
                    // Execute all tasks
                    ice::schedule_tracked_task(task, coro, running, scheduler);
                }

                // We now return from the coroutine if the tasks finished synchronously.
                return running.fetch_sub(1, std::memory_order_relaxed) != 1;
            }

            constexpr bool await_resume() const noexcept
            {
                ICE_ASSERT_CORE(running.load(std::memory_order_relaxed) == 0);
                return ice::span::any(tasks);
            }
        };
        return Awaitable{ tasks, scheduler };
    }

    inline auto internal_await_scheduled_queue(ice::TaskQueue& queue, void* result_ptr, ice::TaskScheduler& scheduler) noexcept
    {
        struct Awaitable
        {
            ice::TaskQueue& queue;
            void* queue_result;
            ice::TaskScheduler& scheduler;
            ice::TaskAwaitableBase awaitable{ ._params = { ice::TaskAwaitableModifier::Unused } };

            inline auto await_ready() const noexcept
            {
                // Only suspend if we actually have tasks
                return queue.empty();
            }

            inline auto await_suspend(std::coroutine_handle<> coro) noexcept
            {
                ice::TaskQueue& scheduler_queue = scheduler.schedule()._queue;

                // We set the result value for each awaitable in the queue and nothing more.
                ice::LinkedQueueRange<ice::TaskAwaitableBase> tasks_awaitables = queue.consume();
                for (ice::TaskAwaitableBase* task_awaitable : tasks_awaitables)
                {
                    ICE_ASSERT_CORE(task_awaitable->result.ptr == nullptr);
                    task_awaitable->result.ptr = queue_result;
                }
                scheduler_queue.push_back(tasks_awaitables);

                // Prepare our own awaitable object and push it onto the scheduler. This ensures it's resumed after all
                //   previously queued awaitables.
                awaitable._coro = coro;
                scheduler_queue.push_back(&awaitable);
            }

            inline bool await_resume() const noexcept
            {
                return (bool) awaitable._coro;
            }
        };
        return Awaitable{ queue, result_ptr, scheduler };
    }

} // namespace ice
