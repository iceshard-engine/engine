/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename Value = void>
    class Task;
    struct TaskQueue;
    class TaskScheduler;

    class TaskThread;
    class TaskThreadPool;

    struct TaskFlags;
    struct TaskAwaitableParams;
    struct TaskAwaitableBase;

    struct TaskContainer;
    class ScopedTaskContainer;

    class ManualResetEvent;
    class ManualResetBarrier;

    // STD aliases
    template<typename Type = void>
    using coroutine_handle = std::coroutine_handle<Type>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;

} // namespace ice
