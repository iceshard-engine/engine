/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_v3.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/span.hxx>

namespace ice
{

    void sync_wait(ice::Task_v3<void> task) noexcept;

    template<typename T>
    auto sync_wait(ice::Task_v3<T> task) noexcept -> T;

    void sync_manual_wait(
        ice::Task_v3<void> task,
        ice::ManualResetEvent& reset_event
    ) noexcept;

    void sync_wait_all(
        ice::Allocator& alloc,
        ice::Span<ice::Task_v3<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept;

    void when_all_ready(
        //ice::Allocator& alloc,
        ice::Span<ice::Task_v3<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept;

    auto sync_task(
        ice::Task_v3<void> task,
        ice::ManualResetEvent* reset_event
    ) noexcept -> ice::Task_v3<>;


    template<typename T>
    auto sync_wait(
        ice::Task_v3<T> task
    ) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](ice::Task_v3<T> awaited_task, T& value) noexcept -> ice::Task_v3<>
        {
            value = co_await awaited_task;
        };
        ice::sync_wait(task_wrapper(ice::move(task), result));
        return result;
    }

} // namespace ice
