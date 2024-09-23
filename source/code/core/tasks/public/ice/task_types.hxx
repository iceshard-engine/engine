/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename Value = void>
    class Task;

    template<typename Result, typename ErrorType = ice::ErrorCode>
    struct TaskExpected;

    class TaskQueue;
    class TaskScheduler;

    class TaskCheckpoint;
    class TaskCheckpointGate;

    class TaskThread;
    class TaskThreadPool;

    struct TaskFlags;
    struct TaskAwaitableParams;
    struct TaskAwaitableBase;

    struct TaskContainer;
    class ScopedTaskContainer;

    class ManualResetEvent;
    class ManualResetBarrier;

    // Generator task
    template<typename Value>
    class Generator;

    // STD aliases
    template<typename Type = void>
    using coroutine_handle = std::coroutine_handle<Type>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;

} // namespace ice
