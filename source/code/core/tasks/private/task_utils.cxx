/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task_utils.hxx>
#include "internal_tasks/task_utils.hxx"

namespace ice
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

    auto await_scheduled(ice::Task<> task, ice::TaskScheduler &scheduler) noexcept -> ice::Task<>
    {
        co_await scheduler;
        co_await task;
    }

    auto await_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        co_await ice::internal_await_scheduled_tasks(tasks, scheduler);
    }

    auto await_scheduled_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<bool>
    {
        co_return co_await ice::internal_await_scheduled_queue(queue, nullptr, scheduler);
    }

    auto await_scheduled_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept -> ice::Task<bool>
    {
        co_return co_await ice::internal_await_scheduled_queue(queue, result, scheduler);
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


    bool execute_task(ice::Task<> task) noexcept
    {
        bool const valid_task = task.valid();
        if (valid_task)
        {
            ice::execute_detached_task(ice::move(task));
        }
        return valid_task;
    }

    bool execute_tasks(ice::Span<ice::Task<>> tasks) noexcept
    {
        for (ice::Task<>& task : tasks)
        {
            ice::execute_detached_task(ice::move(task));
        }
        return ice::span::any(tasks);
    }

    bool schedule_task(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept
    {
        bool const valid_task = task.valid();
        if (valid_task)
        {
            ice::schedule_detached_task(ice::move(task), scheduler);
        }
        return valid_task;
    }

    bool schedule_tasks(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept
    {
        for (ice::Task<>& task : tasks)
        {
            ice::schedule_detached_task(ice::move(task), scheduler);
        }
        return ice::span::any(tasks);
    }

    bool schedule_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept
    {
        ice::TaskQueue& scheduler_queue = scheduler.schedule()._queue;

        // Move the queue task to the given scheduler.
        return scheduler_queue.push_back(queue.consume());
    }

    bool schedule_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept
    {
        // We set the result value for each awaitable in the queue and nothing more.
        ice::LinkedQueueRange<ice::TaskAwaitableBase> tasks_awaitables = queue.consume();
        for (ice::TaskAwaitableBase* task_awaitable : tasks_awaitables)
        {
            ICE_ASSERT_CORE(task_awaitable->result.ptr == nullptr);
            task_awaitable->result.ptr = result;
        }

        ice::TaskQueue& scheduler_queue = scheduler.schedule()._queue;

        // Move the queue task to the given scheduler.
        return scheduler_queue.push_back(tasks_awaitables);
    }

    void manual_wait_for(ice::ManualResetEvent& event, ice::Task<> task) noexcept
    {
        execute_detached_task(ice::move(task), event);
    }

    void manual_wait_for(ice::ManualResetBarrier& barrier, ice::Task<> task) noexcept
    {
        execute_detached_task(ice::move(task), barrier);
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

    void manual_wait_for_scheduled(ice::ManualResetBarrier& barrier, ice::Task<> task, ice::TaskScheduler& scheduler) noexcept
    {
        schedule_detached_task(ice::move(task), scheduler, barrier);
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


    ////////////////////////////////////////////////////////////////


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

} // namespace ice
