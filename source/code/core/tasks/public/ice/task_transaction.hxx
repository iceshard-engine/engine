/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    struct TaskTransaction
    {
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> queue;
        std::atomic<ice::u32> awaiters;
    };

    struct TaskTransactionTracker
    {
        std::atomic<ice::TaskTransaction*> current_transaction = nullptr;

        inline auto start_transaction(ice::TaskTransaction& transaction) noexcept -> ice::TaskTransaction&
        {
            ice::TaskTransaction* ongoing_transaction = nullptr;
            if (current_transaction.compare_exchange_strong(ongoing_transaction, &transaction, std::memory_order_relaxed))
            {
                return transaction;
            }
            return *ongoing_transaction;
        }

        inline void finish_transaction(ice::TaskTransaction& transaction) noexcept
        {
            if (current_transaction.load(std::memory_order_relaxed) == ice::addressof(transaction))
            {
                current_transaction.store(nullptr, std::memory_order_relaxed);
            }
        }
    };

} // namespace ice
