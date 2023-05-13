/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    class TaskQueue
    {
    public:
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> _awaitables;
    };

} // namespace ice
