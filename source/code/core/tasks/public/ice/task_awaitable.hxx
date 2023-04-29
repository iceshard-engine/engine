#pragma once
#include <ice/task_types.hxx>
#include <ice/task_flags.hxx>

namespace ice
{

    enum class TaskAwaitableModifier_v3 : ice::u32
    {
        Unused = 0x0,
        PriorityFlags = 0x8000'0000,
        DelayedExecution = 0x4000'0000,
        CustomValue = 0x2000'0000,
    };

    struct TaskAwaitableParams
    {
        ice::TaskAwaitableModifier_v3 modifier;
        union
        {
            ice::u32 u32_value;
            ice::TaskFlags task_flags;
        };
    };

    struct TaskAwaitableBase
    {
        ice::TaskAwaitableParams const _params;
        ice::coroutine_handle<> _coro;

        ice::TaskAwaitableBase* next;
    };

} // namespace ice
