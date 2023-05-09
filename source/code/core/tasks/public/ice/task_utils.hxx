/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_types.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/span.hxx>

namespace ice
{

    template<typename T>
    inline auto wait_for(ice::Task<T> task) noexcept -> T;
    void wait_for(ice::Task<void> task) noexcept;
    void wait_for_all(ice::Span<ice::Task<void>> task) noexcept;

    template<typename T>
    inline auto manual_wait_for(ice::Task<T> task, ice::ManualResetEvent& manual_event) noexcept -> T;
    void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept;
    void manual_wait_for_all(ice::Span<ice::Task<void>>, ice::ManualResetBarrier& manual_event) noexcept;

    template<typename Value>
    inline auto schedule_on(ice::Task<Value> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>;
    inline auto schedule_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;
    void schedule_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept;
    void schedule_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept;

    template<typename Value>
    inline auto resume_on(ice::Task<Value> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<Value>;
    inline auto resume_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<void>;
    void resume_task_on(ice::Task<void> task, ice::TaskScheduler& scheduler) noexcept;
    void resume_tasks_on(ice::Span<ice::Task<void>> tasks, ice::TaskScheduler& scheduler) noexcept;


    template<typename T>
    inline auto wait_for(ice::Task<T> task) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](ice::Task<T> awaited_task, T& value) noexcept -> ice::Task<void>
        {
            value = co_await awaited_task;
        };
        ice::wait_for(task_wrapper(ice::move(task), result));
        return result;
    }

    template<typename T>
    inline auto manual_wait_for(ice::Task<T> task, ice::ManualResetEvent& manual_event) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](ice::Task<T> awaited_task, T& value) noexcept -> ice::Task<void>
        {
            value = co_await awaited_task;
        };
        ice::manual_wait_for(task_wrapper(ice::move(task), result), manual_event);
        return result;
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
