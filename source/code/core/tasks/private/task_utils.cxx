/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/container/array.hxx>
#include "internal_tasks/task_detached.hxx"
#include "internal_tasks/task_tracked.hxx"

namespace ice
{

    namespace detail
    {

        template<typename T>
        concept HasSetMethod = requires(T t) {
            { t.set() } -> std::convertible_to<void>;
        };

        auto detached_task(ice::Task<void> awaited_task, HasSetMethod auto& manual_reset_sem) noexcept -> ice::DetachedTask
        {
            co_await awaited_task;
            manual_reset_sem.set();
        }

        auto detached_task_schedule(ice::Task<void> scheduled_task, ice::TaskScheduler& scheduler) noexcept -> ice::DetachedTask
        {
            co_await scheduler;
            co_await scheduled_task;

            // [18/06/2024] Fails to call dtor/relase the 'scheduled_task' coroutine on Release builds. Might want to find a repro case and report to MS
            //   See comment in 'detached_task_resume' for details.
            co_await scheduled_task;
        }

        auto detached_task_resume(ice::Task<void> scheduled_task, ice::TaskScheduler& scheduler) noexcept -> ice::DetachedTask
        {
            co_await scheduled_task;
            co_await scheduler;

            // BUG?: For some reason a when scheduled here, and resumed on a thread, this coroutine is not destroyed properly
            //  without additional actions after the scheduling.
            //  To avoid this we await the completed task which is complex enough to resume the coroutine properly but simple to not really impose any cost.
            // TODO: Using this function is already a hack, so we might just want refactor all locations that make use of this weird thing.
            co_await scheduled_task;
        }

        // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
        auto detached_awaiting_task(
            ice::Task<void>& awaited_task,
            std::coroutine_handle<>,
            std::atomic_uint32_t&
        ) noexcept -> ice::TrackedTask
        {
            co_await awaited_task;
        }

        // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
        auto detached_awaiting_task(
            ice::Task<void>& awaited_task,
            std::coroutine_handle<>,
            std::atomic_uint32_t&,
            ice::TaskScheduler& scheduler
        ) noexcept -> ice::TrackedTask
        {
            co_await scheduler;
            co_await awaited_task;
        }

        auto await_tasks(ice::Span<ice::Task<void>> tasks) noexcept
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
                        detail::detached_awaiting_task(task, coro, running);
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

        auto await_tasks_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept
        {
            struct Awaitable
            {
                ice::TaskScheduler& scheduler;
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
                        detail::detached_awaiting_task(task, coro, running, scheduler);
                    }

                    // Steal a task from the queue
                    ice::u32 running_val = running.load(std::memory_order_relaxed);
                    while(running_val > 1)
                    {
                        // Try to process a task, if not possible wait for tasks to finish.
                        if (scheduler.schedule()._queue.process_one() == false)
                        {
                            // Wait for the given value.
                            running.wait(running_val, std::memory_order_relaxed);
                        }

                        // Get new value, if == '1' then we are ready to finalize
                        running_val = running.load(std::memory_order_relaxed);
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
            return Awaitable{ scheduler, tasks };
        }

    } // namespace detail

    // void wait_for(ice::Task<void> task) noexcept
    // {
    //     IPT_ZONE_SCOPED;
    //     ice::ManualResetEvent ev;
    //     manual_wait_for(ev, ice::move(task));
    //     ev.wait();
    // }

    // void wait_for_all(ice::Span<ice::Task<void>> tasks) noexcept
    // {
    //     IPT_ZONE_SCOPED;
    //     ice::ManualResetBarrier manual_sem{ ice::u8(ice::span::count(tasks)) };
    //     manual_wait_for_all(tasks, manual_sem);
    //     manual_sem.wait();
    // }

    // void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept
    // {
    //     IPT_ZONE_SCOPED;
    //     detail::detached_task(ice::move(task), manual_event);
    // }

    // void manual_wait_for_all(ice::Span<ice::Task<void>> tasks, ice::ManualResetBarrier& manual_barrier) noexcept
    // {
    //     IPT_ZONE_SCOPED;
    //     for (ice::Task<void>& task : tasks)
    //     {
    //         detail::detached_task(ice::move(task), manual_barrier);
    //     }
    // }

    void schedule_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept
    {
        detail::detached_task_schedule(ice::move(task), scheduler);
    }

    void schedule_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        for (ice::Task<void>& task : tasks)
        {
            schedule_task_on(ice::move(task), scheduler);
        }
    }

    void resume_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept
    {
        detail::detached_task_resume(ice::move(task), scheduler);
    }

    void resume_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        for (ice::Task<void>& task : tasks)
        {
            resume_task_on(ice::move(task), scheduler);
        }
    }

    auto await_all(ice::Span<ice::Task<void>> tasks) noexcept -> ice::Task<void>
    {
        co_await detail::await_tasks(tasks);
    }

    auto await_all_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>
    {
        co_await detail::await_tasks_scheduled(tasks, scheduler);
    }

    auto await_all_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<void>
    {
        co_await detail::await_tasks(tasks);
        co_await resumer;
    }

    auto await_all_on_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>
    {
        co_await detail::await_tasks_scheduled(tasks, scheduler);
        co_await resumer;
    }

    auto await_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer) noexcept -> ice::Task<bool>
    {
        // We make use of the fact that the awaitable has public access to the queue.
        // This is currently a bit of cheating, but it's not directly accessible to anyone who doesn't know what's happening here ;p
        auto awaitable = resumer.schedule();

        // First we push all awaitables from the queue onto the resumers queue.
        bool const added = awaitable._queue.push_back(queue.consume());

        // Then we await the current coroutine which will be pushed as the last item on the scheduler, so we know all previous task will be processed first.
        // NOTE: Processing might be out of order if the scheduler queue is assigned to multiple threads!
        co_await awaitable;

        // Return information if anything was in the queue
        co_return added;
    }

    auto await_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& resumer) noexcept -> ice::Task<bool>
    {
        // We set the result value for each awaitable in the queue and nothing more.
        auto tasks_awaitables = queue.consume();
        for (ice::TaskAwaitableBase* task_awaitable : tasks_awaitables)
        {
            task_awaitable->result.ptr = result;
        }

        // We make use of the fact that the awaitable has public access to the queue.
        // This is currently a bit of cheating, but it's not directly accessible to anyone who doesn't know what's happening here ;p
        auto awaitable = resumer.schedule();

        // First we push all awaitables from the queue onto the resumers queue. The range is copied, so we can check later if actually any tasks where queued.
        bool const added = awaitable._queue.push_back(ice::move(tasks_awaitables));

        // Then we await the current coroutine which will be pushed as the last item on the scheduler, so we know all previous task will be processed first.
        // NOTE: Processing might be out of order if the scheduler queue is assigned to multiple threads!
        co_await awaitable;

        // Return information if anything was in the queue
        co_return added;
    }

    auto await_filtered_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer, FnTaskQueueFilter filter, void* userdata) noexcept -> ice::Task<bool>
    {
        // We make use of the fact that the awaitable has public access to the queue.
        // This is currently a bit of cheating, but it's not directly accessible to anyone who doesn't know what's happening here ;p
        auto awaitable = resumer.schedule();

        ice::TaskAwaitableBase* last_remaining = nullptr;
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> remaining_tasks;

        // Get all awaiting tasks
        bool result = false;
        for (ice::TaskAwaitableBase* task_awaitable : queue.consume())
        {
            ICE_ASSERT(task_awaitable->_params.modifier == TaskAwaitableModifier::CustomValue, "Unexpected modifier type!");

            if (filter(task_awaitable->_params, userdata))
            {
                result = true;

                // First we push all awaitables from the queue onto the resumers queue. The range is copied, so we can check later if actually any tasks where queued.
                awaitable._queue.push_back(task_awaitable);
            }
            else
            {
                last_remaining = task_awaitable;

                // Pushing awaitiable onto remaining task does not change the 'next' pointer so we don't invalidate this range.
                ice::linked_queue::push(remaining_tasks, task_awaitable);
            }
        }

        // Reset the 'next' ptr as it might contain old values, this also means we need to push remaining tasks back to the asset entry queue.
        // We are safe to do so here because another task loading a "next" asset representation will wait for all awaiting tasks to be pushed.
        if (last_remaining != nullptr)
        {
            last_remaining->next = nullptr;

            // We don't need to find any 'next' pointer here fortunately
            queue.push_back(ice::linked_queue::consume(remaining_tasks));
        }

        co_await awaitable;

        co_return result;
    }

    bool schedule_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer) noexcept
    {
        // We make use of the fact that the awaitable has public access to the queue.
        // This is currently a bit of cheating, but it's not directly accessible to anyone who doesn't know what's happening here ;p
        auto awaitable = resumer.schedule();

        // First we push all awaitables from the queue onto the resumers queue.
        return awaitable._queue.push_back(queue.consume());
    }

    bool schedule_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& resumer) noexcept
    {
        // We set the result value for each awaitable in the queue and nothing more.
        auto tasks_awaitables = queue.consume();
        for (ice::TaskAwaitableBase* task_awaitable : tasks_awaitables)
        {
            task_awaitable->result.ptr = result;
        }

        // We make use of the fact that the awaitable has public access to the queue.
        // This is currently a bit of cheating, but it's not directly accessible to anyone who doesn't know what's happening here ;p
        auto awaitable = resumer.schedule();

        // First we push all awaitables from the queue onto the resumers queue.
        return awaitable._queue.push_back(ice::move(tasks_awaitables));
    }

} // namespace ice
