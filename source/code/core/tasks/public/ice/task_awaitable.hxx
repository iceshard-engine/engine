/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_flags.hxx>

namespace ice
{

    enum class TaskAwaitableModifier : ice::u32
    {
        Unused = 0x0,
        PriorityFlags = 0x8000'0000,
        DelayedExecution = 0x4000'0000,
        CustomValue = 0x2000'0000,
    };

    struct TaskAwaitableParams
    {
        ice::TaskAwaitableModifier modifier;
        union
        {
            ice::u32 u32_value;
            ice::TaskFlags task_flags;
        };
    };

    struct TaskAwaitableResult
    {
        void* ptr;
    };

    struct TaskAwaitableBase
    {
        ice::TaskAwaitableParams const _params;
        ice::coroutine_handle<> _coro;
        ice::TaskAwaitableBase* next;
        ice::TaskAwaitableResult result;
    };

} // namespace ice
