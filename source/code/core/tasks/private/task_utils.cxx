/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    namespace detail
    {

        struct DetachedTask
        {
            struct promise_type
            {
                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() const noexcept { return std::suspend_never{ }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept { return DetachedTask{ }; }
                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unexpected coroutine exception!");
                }
            };
        };

        template<typename T>
        concept HasSetMethod = requires(T t) {
            { t.set() } -> std::convertible_to<void>;
        };

        auto detached_task(ice::Task<void> awaited_task, HasSetMethod auto& manual_reset_sem) noexcept -> DetachedTask
        {
            co_await awaited_task;
            manual_reset_sem.set();
        }

        auto detached_task_schedule(ice::Task<void> scheduled_task, ice::TaskScheduler& scheduler) noexcept -> DetachedTask
        {
            co_await scheduler;
            co_await scheduled_task;

            // [18/06/2024] Fails to call dtor/relase the 'scheduled_task' coroutine on Release builds. Might want to find a repro case and report to MS
            //   See comment in 'detached_task_resume' for details.
            co_await scheduled_task;
        }

        auto detached_task_resume(ice::Task<void> scheduled_task, ice::TaskScheduler& scheduler) noexcept -> DetachedTask
        {
            co_await scheduled_task;
            co_await scheduler;

            // BUG?: For some reason a when scheduled here, and resumed on a thread, this coroutine is not destroyed properly
            //  without additional actions after the scheduling.
            //  To avoid this we await the completed task which is complex enough to resume the coroutine properly but simple to not really impose any cost.
            // TODO: Using this function is already a hack, so we might just want refactor all locations that make use of this weird thing.
            co_await scheduled_task;
        }


        class DetachedAwaitingTask
        {
        public:
            DetachedAwaitingTask() noexcept = default;

            struct promise_type
            {
                // All tasks have the same continuation
                std::coroutine_handle<> _continuation;

                // However only the last one remaining will 'not' be ready and return the continuation in 'await_suspend'
                std::atomic_uint32_t& _remaining;

                promise_type(ice::Task<> const&, std::coroutine_handle<> continuation, std::atomic_uint32_t& remaining) noexcept
                    : _continuation{ continuation }
                    , _remaining{ remaining }
                { }

                struct final_awaitable
                {
                    promise_type* promise;

                    // On the final suspend point we are checking checking if we are the last remaining task
                    inline auto await_ready() const noexcept { return promise->_remaining.fetch_sub(1, std::memory_order_release) > 1; }

                    // If so we return the continuation
                    inline auto await_suspend(std::coroutine_handle<> coro) const noexcept -> std::coroutine_handle<>
                    {
                        std::atomic_thread_fence(std::memory_order_acquire);

                        // Can't access promise after it's destroyed
                        std::coroutine_handle<> continuation = promise->_continuation;

                        // We delete the detached coroutine here, since this would be normaly done when 'await_ready == true' after resuming
                        //  from the final suspension point. Since we are suspending, but we never resume again we just delete it here.
                        // TODO: Run more tests that this is no longer needed
                        // coro.destroy();

                        // Return the continuation
                        return continuation;
                    }

                    constexpr void await_resume() const noexcept { }
                };

                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() noexcept { return final_awaitable{ this }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept -> DetachedAwaitingTask
                {
                    return DetachedAwaitingTask{};
                }

                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unexpected coroutine exception!");
                }
            };
        };

        // We have this large signature to pass the coroutine handle and atomic counter to the promise type with the same ctor signature.
        auto detached_awaiting_task(
            ice::Task<void> awaited_task,
            std::coroutine_handle<>,
            std::atomic_uint32_t&
        ) noexcept -> DetachedAwaitingTask
        {
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
                        detail::detached_awaiting_task(ice::move(task), coro, running);
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

    } // namespace detail

    void wait_for(ice::Task<void> task) noexcept
    {
        IPT_ZONE_SCOPED;
        ice::ManualResetEvent ev;
        manual_wait_for(ice::move(task), ev);
        ev.wait();
    }

    void wait_for_all(ice::Span<ice::Task<void>> tasks) noexcept
    {
        IPT_ZONE_SCOPED;
        ice::ManualResetBarrier manual_sem{ ice::u8(ice::span::count(tasks)) };
        manual_wait_for_all(tasks, manual_sem);
        manual_sem.wait();
    }

    void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept
    {
        IPT_ZONE_SCOPED;
        detail::detached_task(ice::move(task), manual_event);
    }

    void manual_wait_for_all(ice::Span<ice::Task<void>> tasks, ice::ManualResetBarrier& manual_barrier) noexcept
    {
        IPT_ZONE_SCOPED;
        for (ice::Task<void>& task : tasks)
        {
            detail::detached_task(ice::move(task), manual_barrier);
        }
    }

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
        ice::TaskQueue queue;
        ice::TaskScheduler queue_scheduler{ queue };
        ice::schedule_tasks_on(ice::move(tasks), queue_scheduler);

        // Awaits the tasks directly on the scheduler queue
        co_await ice::await_queue_on(queue, scheduler);
    }

    auto await_all_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<void>
    {
        co_await detail::await_tasks(tasks);
        co_await resumer;
    }

    auto await_all_on_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>
    {
        ice::TaskQueue queue;
        ice::TaskScheduler queue_scheduler{ queue };
        ice::schedule_tasks_on(ice::move(tasks), queue_scheduler);

        // Awaits the tasks directly on the scheduler queue
        co_await ice::await_queue_on(queue, scheduler);
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
