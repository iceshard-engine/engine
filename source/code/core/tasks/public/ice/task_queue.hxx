/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    struct TaskQueue final
    {
        template<typename Value>
        inline void process_one(Value& result_value) noexcept;
        inline void process_one(void* result_value = nullptr) noexcept;

        template<typename Value>
        inline auto process_all(Value& result_value) noexcept -> ice::ucount;
        inline auto process_all(void* result_value = nullptr) noexcept -> ice::ucount;

        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> _awaitables;
    };

    template<typename Value>
    inline void TaskQueue::process_one(Value& result_value) noexcept
    {
        this->process_one(reinterpret_cast<void*>(&result_value));
    }

    inline void TaskQueue::process_one(void* result_value) noexcept
    {
        ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(_awaitables);
        if (awaitable != nullptr)
        {
            awaitable->result.ptr = result_value;
            awaitable->_coro.resume();
        }
    }

    template<typename Value>
    inline auto TaskQueue::process_all(Value& result_value) noexcept -> ice::ucount
    {
        this->process_all(reinterpret_cast<void*>(&result_value));
    }

    inline auto TaskQueue::process_all(void* result_value) noexcept -> ice::ucount
    {
        ice::ucount processed = 0;
        for (ice::TaskAwaitableBase* const awaitable : ice::linked_queue::consume(_awaitables))
        {
            awaitable->result.ptr = result_value;
            awaitable->_coro.resume();
            processed += 1;
        }
        return processed;
    }

} // namespace ice
