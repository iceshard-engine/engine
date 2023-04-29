#pragma once
#include <ice/task_types_v3.hxx>
#include <ice/task_flags_v3.hxx>

namespace ice
{

    enum class TaskAwaitableModifier_v3 : ice::u32
    {
        Unused = 0x0,
        PriorityFlags = 0x8000'0000,
        DelayedExecution = 0x4000'0000,
        CustomValue = 0x2000'0000,
    };

    struct TaskAwaitableParams_v3
    {
        ice::TaskAwaitableModifier_v3 modifier;
        union
        {
            ice::u32 u32_value;
            ice::TaskFlags task_flags;
        };
    };

    struct TaskAwaitableBase_v3
    {
        ice::TaskAwaitableParams_v3 const _params;
        ice::coroutine_handle<> _coro;

        ice::TaskAwaitableBase_v3* next;
    };

    //class TaskAwaitable_v3 : TaskAwaitableBase_v3
    //{
    //public:
    //    using TaskAwaitableBase_v3::TaskAwaitableBase_v3;

    //    bool await_ready() const noexcept { return false; }
    //    void await_resume() const noexcept { }
    //};

} // namespace ice
