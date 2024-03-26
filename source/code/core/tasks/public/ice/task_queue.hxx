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
        inline bool any() const noexcept { return ice::linked_queue::any(_awaitables); }
        inline bool empty() const noexcept { return ice::linked_queue::empty(_awaitables); }

        template<typename Value>
        inline bool process_one(Value& result_value) noexcept;
        inline bool process_one(void* result_value = nullptr) noexcept;

        template<typename Value>
        inline auto process_all(Value& result_value) noexcept -> ice::ucount;
        inline auto process_all(void* result_value = nullptr) noexcept -> ice::ucount;

        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> _awaitables;
    };

    template<typename Value>
    inline bool TaskQueue::process_one(Value& result_value) noexcept
    {
        return this->process_one(reinterpret_cast<void*>(&result_value));
    }

    inline bool TaskQueue::process_one(void* result_value) noexcept
    {
        ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(_awaitables);
        if (awaitable != nullptr)
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }
            awaitable->_coro.resume();
        }
        return awaitable != nullptr;
    }

    template<typename Value>
    inline auto TaskQueue::process_all(Value& result_value) noexcept -> ice::ucount
    {
        return this->process_all(reinterpret_cast<void*>(&result_value));
    }

    inline auto TaskQueue::process_all(void* result_value) noexcept -> ice::ucount
    {
        ice::ucount processed = 0;
        for (ice::TaskAwaitableBase* const awaitable : ice::linked_queue::consume(_awaitables))
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }
            awaitable->_coro.resume();
            processed += 1;
        }
        return processed;
    }

} // namespace ice
