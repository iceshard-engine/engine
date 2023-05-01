/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/span.hxx>

namespace ice
{

    void wait_for(ice::Task<void> task) noexcept;
    void wait_for_all(ice::Span<ice::Task<void>> task) noexcept;

    void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept;
    void manual_wait_for_all(ice::Span<ice::Task<void>>, ice::ManualResetSemaphore& manual_event) noexcept;

    template<typename T>
    auto wait_for(ice::Task<T> task) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](ice::Task<T> awaited_task, T& value) noexcept -> ice::Task<void>
        {
            value = co_await awaited_task;
        };
        ice::wait_for(task_wrapper(ice::move(task), result));
        return result;
    }

} // namespace ice
