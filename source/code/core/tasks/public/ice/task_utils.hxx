/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_types.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/span.hxx>
#include <ice/profiler.hxx>
#include <ice/expected.hxx>

namespace ice
{

    // Awaiters

    // Resumes the current task on a different thread
    inline auto resume_on(ice::TaskScheduler& scheduler) noexcept;

    // Executes tasks (1) on this thread, waits for finish and resumes (2) on given thread

    inline auto await_tasks(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>;

    auto await_on(ice::Task<> task, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    template<typename T>
    inline auto await_on(ice::Task<T> task, ice::TaskScheduler& resumer) noexcept -> ice::Task<T>;

    // Schedules (2) tasks (1) and resumes after all have been finished (resuming on unspecified thread)

    auto await_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    auto await_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<>;

    template<typename T>
    inline auto await_scheduled(ice::Task<T> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<T>;

    // Schedules (2) queue (1) and resumes after all have been scheduled (tasks might not have started or finished, resuming on scheduler thread)

    auto await_scheduled_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept -> ice::Task<bool>;

    auto await_scheduled_queue(ice::TaskQueue& queue, void* result_ptr, ice::TaskScheduler& scheduler) noexcept -> ice::Task<bool>;

    // Schedules (2) tasks (1) and resumes on resumer (3) after all have finished

    auto await_scheduled_on(ice::Task<> task, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    template<typename T>
    inline auto await_scheduled_on(ice::Task<> task, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<T>;

    auto await_scheduled_on(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_scheduled_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;

    auto await_scheduled_queue_on(ice::TaskQueue& queue, void* result_ptr, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>;


    // Detached schedulers

    bool execute_task(ice::Task<> task) noexcept;

    bool execute_tasks(ice::Span<ice::Task<>> tasks) noexcept;

    // Schedules (2) tasks (1) and resumes after all have been scheduled (tasks might not have finished, resuming on origin thread)

    bool schedule_task(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;

    bool schedule_tasks(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept;

    // Schedules (2) queue (1) and resumes after all have been scheduled (tasks might not have finished, resuming on origin thread)

    bool schedule_queue(ice::TaskQueue& queue, ice::TaskScheduler& scheduler) noexcept;

    bool schedule_queue(ice::TaskQueue& queue, void* result_ptr, ice::TaskScheduler& scheduler) noexcept;


    // Waiters

    void wait_for(ice::Task<> task) noexcept;
    void wait_for(ice::Span<ice::Task<>> tasks) noexcept;
    void wait_for_scheduled(ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;
    void wait_for_scheduled(ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept;

    template<typename T>
    inline auto wait_for_result(ice::Task<T> task) noexcept -> T;
    template<typename T>
    inline void wait_for_result(ice::Task<T> task, T& out_result) noexcept;
    template<typename T>
    inline void wait_for_result(ice::Span<ice::Task<T>> tasks, ice::Span<T> out_results) noexcept;
    template<typename T>
    inline void wait_for_result_scheduled(ice::Task<T> task, ice::TaskScheduler& scheduler, T& out_result) noexcept;
    template<typename T>
    inline void wait_for_result_scheduled(ice::Span<ice::Task<T>> tasks, ice::TaskScheduler& scheduler, ice::Span<T> out_results) noexcept;

    void manual_wait_for(ice::ManualResetEvent& evnt, ice::Task<> task) noexcept;
    void manual_wait_for(ice::ManualResetBarrier& evnt, ice::Task<> task) noexcept;
    void manual_wait_for(ice::ManualResetBarrier& evnt, ice::Span<ice::Task<>> tasks) noexcept;
    void manual_wait_for_scheduled(ice::ManualResetEvent& evnt, ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;
    void manual_wait_for_scheduled(ice::ManualResetBarrier& evnt, ice::Task<> task, ice::TaskScheduler& scheduler) noexcept;
    void manual_wait_for_scheduled(ice::ManualResetBarrier& evnt, ice::Span<ice::Task<>> tasks, ice::TaskScheduler& scheduler) noexcept;


    ////////////////////////////////////////////////////////////////

    template<typename T>
    inline auto wait_for_expected(ice::TaskExpected<T> task) noexcept -> ice::Expected<T>;

    ////////////////////////////////////////////////////////////////


    [[deprecated("To be replaced at a later time")]]
    auto await_filtered_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer, FnTaskQueueFilter filter, void* userdata = nullptr) noexcept -> ice::Task<bool>;

} // namespace ice

#include "impl/task_utils.inl"
