/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task_queue.hxx>

namespace ice
{

    TaskQueue::TaskQueue(ice::TaskFlags flags) noexcept
        : flags{ flags }
        , _awaitables{ }
    {
    }

    bool TaskQueue::push_back(ice::TaskAwaitableBase* awaitable) noexcept
    {
        ice::linked_queue::push(_awaitables, awaitable);
        _awaitables._head.notify_one();
        return true;
    }

    bool TaskQueue::push_back(ice::LinkedQueueRange<ice::TaskAwaitableBase> awaitable_range) noexcept
    {
        bool const result = ice::linked_queue::push(_awaitables, awaitable_range);
        _awaitables._head.notify_all();
        return result;
    }

    bool TaskQueue::contains(ice::TaskAwaitableBase* awaitable) const noexcept
    {
        auto* volatile it = _awaitables._head.load(std::memory_order_relaxed);
        if (it != nullptr)
        {
            auto* const end = _awaitables._tail.load(std::memory_order_relaxed);

            // Loop when multiple awaitables where pushed after setting the test.
            while(it != end && it != awaitable)
            {
                // We wait for next pointer to be updated
                while(it->next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                it = it->next;
            }

            // Found our awaitable don't suspend
            if (it == awaitable)
            {
                return false;
            }
        }
        return true;
    }

    auto TaskQueue::consume() noexcept -> ice::LinkedQueueRange<ice::TaskAwaitableBase>
    {
        return ice::linked_queue::consume(_awaitables);
    }

    auto TaskQueue::pop() noexcept -> ice::TaskAwaitableBase*
    {
        return ice::linked_queue::pop(_awaitables);
    }

    bool TaskQueue::process_one(void* result_value) noexcept
    {
        ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(_awaitables);
        if (awaitable != nullptr)
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }

            // Support custom resumer logic
            if (awaitable->_params.modifier == ice::TaskAwaitableModifier::CustomResumer)
            {
                void* resumer_ptr = awaitable->result.ptr;
                ICE_ASSERT_CORE(resumer_ptr != nullptr && result_value == nullptr);

                ice::TaskAwaitableCustomResumer const* custom_resumer = reinterpret_cast<ice::TaskAwaitableCustomResumer*>(resumer_ptr);
                if (custom_resumer->fn_resumer(custom_resumer->ud_resumer, *awaitable) == false)
                {
                    // Reset next pointer before puttng back onto the queue
                    awaitable->next = nullptr;

                    // Push back at the end of the queue
                    ice::linked_queue::push(_awaitables, awaitable);
                    return false;
                }
            }

            awaitable->_coro.resume();
        }
        return awaitable != nullptr;
    }

    auto TaskQueue::process_all(void* result_value) noexcept -> ice::ucount
    {
        ice::ucount processed = 0;
        for (ice::TaskAwaitableBase* const awaitable : ice::linked_queue::consume(_awaitables))
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }

            // Support custom resumer logic
            if (awaitable->_params.modifier == ice::TaskAwaitableModifier::CustomResumer)
            {
                void* resumer_ptr = awaitable->result.ptr;
                ICE_ASSERT_CORE(resumer_ptr != nullptr && result_value == nullptr);

                ice::TaskAwaitableCustomResumer const* custom_resumer = reinterpret_cast<ice::TaskAwaitableCustomResumer*>(resumer_ptr);
                if (custom_resumer->fn_resumer(custom_resumer->ud_resumer, *awaitable) == false)
                {
                    // Reset next pointer before puttng back onto the queue
                    awaitable->next = nullptr;

                    // Push back at the end of the queue
                    ice::linked_queue::push(_awaitables, awaitable);
                    continue;
                }
            }

            awaitable->_coro.resume();
            processed += 1;
        }
        return processed;
    }

    // auto TaskQueue::prepare_all(void *result_value) noexcept -> ice::ucount
    // {
    //     return ice::ucount();
    // }

    void TaskQueue::wait_any() noexcept
    {
        _awaitables._head.wait(nullptr, std::memory_order_relaxed);
    }

} // namespace ice
