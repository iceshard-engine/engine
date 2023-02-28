#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename Value = void>
    class Task_v3;
    class TaskQueue_v3;
    class TaskScheduler_v3;
    class TaskAwaitable_v3;

    class TaskThread_v3;
    class TaskThreadPool_v3;

    struct TaskFlags;
    struct TaskAwaitableParams_v3;
    struct TaskAwaitableBase_v3;

    // STD aliases
    template<typename Type = void>
    using coroutine_handle = std::coroutine_handle<Type>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;

} // namespace ice
