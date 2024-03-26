/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_types.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/span.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    //! \brief Executes the given task and blocks until finished.
    //! \param[in] task A valid task object to be executed.
    //! \returns Result of the executed task.
    template<typename T>
    inline auto wait_for(ice::Task<T> task) noexcept -> T;

    //! \brief Executes the given task and blocks until finished.
    //! \param[in] task Valid task object to be executed.
    void wait_for(ice::Task<void> task) noexcept;

    //! \brief Executes the given list of tasks and blocks until all finish.
    //! \param[in] tasks List of valid task objects to be executed.
    void wait_for_all(ice::Span<ice::Task<void>> tasks) noexcept;

    //! \brief Executes the given task and returns immediately. The passed result variable is not valid until the task finishes.
    //! \note The passed 'ManualResetEvent' object will be notifed when the task finishes execution.
    //! \param[in] task Valid task object to be executed.
    //! \param[inout] manual_event Synchronization event object to be notified when the task finishes.
    //! \param[out] out_result Variable where to store the task result.
    template<typename T>
    inline void manual_wait_for(ice::Task<T> task, ice::ManualResetEvent& manual_event, T& out_result) noexcept;

    //! \brief Executes the given task and returns immediately.
    //! \note The passed 'ManualResetEvent' object will be notifed when the task finishes execution.
    //! \param[in] task Valid task object to be executed.
    //! \param[inout] manual_event Synchronization event object to be notified when the task finishes.
    void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept;

    //! \brief Executes the given task and returns immediately.
    //! \note The passed 'ManualResetEvent' object will be notifed when all tasks finish execution.
    //! \param[in] tasks List of valid task objects to be executed.
    //! \param[inout] manual_barrier Synchronization event object to be notified when all tasks finish.
    void manual_wait_for_all(ice::Span<ice::Task<void>>, ice::ManualResetBarrier& manual_barrier) noexcept;

    //! \brief Helper task to await another task after it's scheduled on the given thread.
    //! \param[in] task Valid task object to be awaited.
    //! \param[in] scheduler Scheduler object for the target thread.
    template<typename Value>
    inline auto schedule_on(ice::Task<Value> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>;

    //! \brief Helper task to await another task after it's scheduled on the given thread.
    //! \param[in] task Valid task object to be awaited.
    //! \param[in] scheduler Scheduler object for the target thread.
    inline auto schedule_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;

    //! \brief Helper task to await list of tasks.
    //! \warning The awaiting task is resumed on the thread of the last finished task.
    //! \param[in] tasks List of tasks to be awaited for completion.
    auto await_all(ice::Span<ice::Task<void>> tasks) noexcept -> ice::Task<void>;

    //! \brief Helper task to await list of tasks. The helper task will be scheduled on the provided thread before tasks are executed.
    //! \warning The awaiting task is resumed on the thread of the last finished task.
    //! \param[in] tasks List of tasks to be awaited for completion.
    //! \param[in] scheduler Scheduler object transfering execution to associated threads.
    auto await_all_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;

    //! \brief Helper task to await list of tasks. Resumes the awaiting task on the resumer thread.
    //! \note Tasks will be executed on the current thread.
    //! \param[in] tasks List of tasks to be awaited for completion.
    //! \param[in] resumer Resumer object transfering execution to associated threads after.
    auto await_all_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer) noexcept -> ice::Task<void>;

    //! \brief Helper task to await list of tasks.
    //! \note The helper task will be scheduled on the provided thread before tasks are executed.
    //! \note Resumes the awaiting task on the resumer thread.
    //! \param[in] tasks List of tasks to be awaited for completion.
    //! \param[in] resumer Resumer object transfering execution to associated threads after.
    //! \param[in] scheduler Scheduler object transfering execution to associated threads.
    auto await_all_on_scheduled(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& resumer, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;

    auto await_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer) noexcept -> ice::Task<bool>;

    auto await_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& resumer) noexcept -> ice::Task<bool>;

    bool schedule_queue_on(ice::TaskQueue& queue, ice::TaskScheduler& resumer) noexcept;

    bool schedule_queue_on(ice::TaskQueue& queue, void* result, ice::TaskScheduler& resumer) noexcept;


    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    void schedule_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept;

    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    void schedule_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept;

    template<typename Value>
    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    inline auto resume_on(ice::Task<Value> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>;

    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    inline auto resume_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;

    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    void resume_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept;

    [[deprecated("Currently it's unsure if this function will remain in the current form.")]]
    void resume_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept;

    template<typename T>
    inline auto wait_for(ice::Task<T> task) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](ice::Task<T> awaited_task, T& value) noexcept -> ice::Task<void>
        {
            value = ice::move(co_await awaited_task);
        };
        ice::wait_for(task_wrapper(ice::move(task), result));
        return result;
    }

    template<typename T>
    inline void manual_wait_for(ice::Task<T> task, ice::ManualResetEvent& manual_event, T& out_result) noexcept
    {
        auto const task_wrapper = [](ice::Task<T> awaited_task, T& value) noexcept -> ice::Task<void>
        {
            value = co_await awaited_task;
        };
        ice::manual_wait_for(task_wrapper(ice::move(task), out_result), manual_event);
    }

    inline auto resume_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>
    {
        co_await task;
        co_await scheduler;
    }

    inline auto schedule_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>
    {
        co_await scheduler;
        co_await task;
    }

    template<typename Value>
    inline auto schedule_on(ice::Task<Value> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>
    {
        co_await scheduler;
        co_return co_await std::move(task);
    }

    template<typename Value>
    inline auto resume_on(ice::Task<Value>&& task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>
    {
        Value value = co_await std::move(task);
        co_await scheduler;
        co_return std::move(value);
    }

} // namespace ice
