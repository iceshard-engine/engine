#pragma once
#include <ice/task_types_v3.hxx>
#include <ice/task_awaitable_v3.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    class TaskQueue_v3
    {
    public:
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase_v3> _awaitables;
    };

} // namespace ice
