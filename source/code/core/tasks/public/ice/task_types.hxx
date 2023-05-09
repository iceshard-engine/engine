#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename Value = void>
    class Task;
    class TaskQueue;
    class TaskScheduler;

    class TaskThread;
    class TaskThreadPool;

    struct TaskFlags;
    struct TaskAwaitableParams;
    struct TaskAwaitableBase;

    class ManualResetEvent;
    class ManualResetBarrier;

    // STD aliases
    template<typename Type = void>
    using coroutine_handle = std::coroutine_handle<Type>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;

} // namespace ice
