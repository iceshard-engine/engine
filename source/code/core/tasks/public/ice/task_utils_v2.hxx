#pragma once
#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::v2
{

    // Resumes the current task on a different thread
    inline auto resume_on(ice::TaskScheduler& scheduler) noexcept;

    // Executes tasks (1) on this thread, waits for finish and resumes (2) on given thread

    inline auto await_all(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>;

    auto await_on(ice::Task<> task, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    // Schedules (2) tasks (1) and resumes after all have been scheduled (tasks might not have finished, resuming on origin thread)

    auto schedule(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto schedule(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    // Schedules (2) queue (1) and resumes after all have been scheduled (tasks might not have finished, resuming on origin thread)

    auto schedule_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto schedule_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    // Schedules (2) tasks (1) and resumes after all have been finished (resuming on unspecified thread)

    auto await_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto await_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    // Schedules (2) queue (1) and resumes after all have been scheduled (tasks might not have started or finished, resuming on scheduler thread)

    auto await_scheduled_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto await_scheduled_queue(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    // Schedules (2) tasks (1) and resumes on resumer (3) after all have finished

    auto await_scheduled_on(ice::Task<> task, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_scheduled_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_scheduled_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_scheduled_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    // Schedules (2) tasks (1) on scheduler thread and current thread. Resumes on current thread

#if 0
    //auto process_all(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>;

    auto process_all_scheduled(ice::Span<ice::Task<>>, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    //auto process_queue(ice::TaskQueue& queue, void* result = nullptr) noexcept -> ice::Task<>;

    auto process_queue_scheduled(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto process_queue_scheduled(ice::TaskQueue& queue, void* result_ptr, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    // Parallel for each
    template<typename Value, typename Fn>
    auto parallel_for_each(ice::Span<Value> values, ice::ucount batch_size, Fn fn, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>
    {
        ice::TaskQueue local_queue{};
        ice::TaskScheduler local_sched{ local_queue };

        auto const local_task = [](ice::Span<Value> sub_values, Fn& fn) noexcept -> ice::Task<>
        {
            fn(sub_values);
            co_return;
        };

        ice::u32 offset = 0;
        ice::ucount remaining_size = ice::span::count(values);
        while(remaining_size > batch_size)
        {
            co_await schedule_on(local_task(ice::span::subspan(values, offset, batch_size)), local_sched);
            remaining_size -= batch_size;
            offset += batch_size;
        }

        // Final task
        co_await schedule_on(local_task(ice::span::subspan(values, offset, remaining_size)), local_sched);

        // Process all enqueued tasks
        co_await process_queue_scheduled(local_queue, scheduler);
    }
#endif

    // Waiters

    void wait_for(ice::Task<> task) noexcept;
    void wait_for(ice::Span<ice::Task<>> tasks) noexcept;
    void wait_for_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;
    void wait_for_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept;

    void manual_wait_for(ice::ManualResetEvent& evnt, ice::Task<> task) noexcept;
    void manual_wait_for(ice::ManualResetBarrier& evnt, ice::Span<ice::Task<>> tasks) noexcept;
    void manual_wait_for_scheduled(ice::ManualResetEvent& evnt, ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;
    void manual_wait_for_scheduled(ice::ManualResetBarrier& evnt, ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept;

} // namespace ice::v2

#include "impl/task_utils_v2.inl"
