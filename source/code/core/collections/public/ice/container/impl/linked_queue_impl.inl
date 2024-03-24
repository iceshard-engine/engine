/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    inline AtomicLinkedQueue<NodeType>::AtomicLinkedQueue() noexcept
        : _head{ }
        , _tail{ }
    {
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    inline AtomicLinkedQueue<NodeType>::AtomicLinkedQueue(AtomicLinkedQueue&& other) noexcept
        : _head{ }
        , _tail{ }
    {
        // We move atomically to this object
        ice::LinkedQueueRange<NodeType> range = ice::linked_queue::consume(other);
        _head = range._head;
        _tail = range._tail;
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    inline auto AtomicLinkedQueue<NodeType>::operator=(AtomicLinkedQueue&& other) noexcept -> AtomicLinkedQueue&
    {
        if (this != &other)
        {
            // We move atomically to this object
            ice::LinkedQueueRange<NodeType> range = ice::linked_queue::consume(other);
            _head = range._head;
            _tail = range._tail;
        }
        return *this;
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    constexpr auto LinkedQueueRange<NodeType>::Iterator::operator*() const noexcept -> NodeType*
    {
        return _current;
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    constexpr void LinkedQueueRange<NodeType>::Iterator::operator++() noexcept
    {
        if (_current != _tail)
        {
            ICE_ASSERT_CORE(_current != nullptr);
            _current = _next;

            if (_current != _tail)
            {
                // TODO: Could be removed in non atomic linked queues become a thing.
                // NOTE: Because we know that the 'next->next' pointer might change value from a different thread,
                //   it needs to be marked as volatile.
                volatile NodeType* next = _next;
                while (next->next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                _next = _next->next;
            }
        }
        else
        {
            ICE_ASSERT_CORE(_current == _tail);
            _current = nullptr;
        }
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    constexpr bool LinkedQueueRange<NodeType>::Iterator::operator==(Iterator other) const noexcept
    {
        return (_tail == other._tail) && (_current == other._current);
    }

    template<typename NodeType> requires ice::LinkedListNode<NodeType>
    constexpr bool LinkedQueueRange<NodeType>::Iterator::operator!=(Iterator other) const noexcept
    {
        return !(*this == other);
    }

    namespace linked_queue
    {

        template<typename NodeType>
        constexpr bool any(ice::LinkedQueueRange<NodeType> const& queue_range) noexcept
        {
            return queue_range._head != nullptr;
        }

        template<typename NodeType>
        constexpr bool empty(ice::LinkedQueueRange<NodeType> const& queue_range) noexcept
        {
            return queue_range._head == nullptr;
        }

        template<typename NodeType>
        constexpr auto begin(
            ice::LinkedQueueRange<NodeType> const& queue_range
        ) noexcept -> typename ice::LinkedQueueRange<NodeType>::Iterator
        {
            if (queue_range._head != nullptr)
            {
                if (queue_range._head == queue_range._tail)
                {
                    return { queue_range._head, nullptr, queue_range._tail };
                }

                // NOTE: Wait for the first 'next' pointer to be set from a different thread.
                volatile NodeType* next = queue_range._head;
                while (next->next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                return { queue_range._head, queue_range._head->next, queue_range._tail };
            }
            else
            {
                return end(queue_range);
            }
        }

        template<typename NodeType>
        constexpr auto end(
            ice::LinkedQueueRange<NodeType> const& queue_range
        ) noexcept -> typename ice::LinkedQueueRange<NodeType>::Iterator
        {
            return { nullptr, nullptr, queue_range._tail };
        }


        template<typename NodeType>
        inline bool any(ice::AtomicLinkedQueue<NodeType> const& queue) noexcept
        {
            return std::atomic_load_explicit(&queue._head, std::memory_order_relaxed) != nullptr;
        }

        template<typename NodeType>
        inline bool empty(ice::AtomicLinkedQueue<NodeType> const& queue) noexcept
        {
            return std::atomic_load_explicit(&queue._head, std::memory_order_relaxed) == nullptr;
        }

        template<typename NodeType, typename DerivedNodeType>
        inline void push(ice::AtomicLinkedQueue<NodeType>& queue, DerivedNodeType* node) noexcept
        {
            NodeType* const previous_tail = std::atomic_exchange_explicit(
                &queue._tail, node, std::memory_order_relaxed
            );

            if (previous_tail == nullptr)
            {
                std::atomic_store_explicit(
                    &queue._head, node, std::memory_order_relaxed
                );
            }
            else
            {
                previous_tail->next = node;
            }

            std::atomic_thread_fence(std::memory_order_release);
        }

        template<typename NodeType, typename DerivedNodeType>
        inline bool push(ice::AtomicLinkedQueue<NodeType>& queue, ice::LinkedQueueRange<DerivedNodeType> range) noexcept
        {
            // If not tail, then the range is empty
            if (range._tail == nullptr)
            {
                return false;
            }

            // If we have a tail we need to also have a head.
            ICE_ASSERT_CORE(range._head != nullptr);

            // Appending a range is a simple as adding one node.
            // - We take the tail of the range and push it as the new tail on the queue
            // - We set the previous_tail->next pointer to the queue head or set is as the new head.
            // - All other values are still intact and the operation is still atomic like it was in the single node case.

            NodeType* const previous_tail = std::atomic_exchange_explicit(
                &queue._tail, range._tail, std::memory_order_relaxed
            );

            if (previous_tail == nullptr)
            {
                std::atomic_store_explicit(
                    &queue._head, range._head, std::memory_order_relaxed
                );
            }
            else
            {
                previous_tail->next = range._head;
            }

            std::atomic_thread_fence(std::memory_order_release);
            return true;
        }

        template<typename NodeType>
        inline auto pop(ice::AtomicLinkedQueue<NodeType>& queue) noexcept -> NodeType*
        {
            NodeType* result = std::atomic_exchange_explicit(
                &queue._head, nullptr, std::memory_order_relaxed
            );

            if (result == nullptr)
            {
                // TODO: spin 100 tries to get something if tail != nullptr.
            }

            if (result != nullptr)
            {
                NodeType* tail_node = std::atomic_load_explicit(
                    &queue._tail, std::memory_order_relaxed
                );

                // We 'fail' be default because `if (tail != head)` we need to update the head.
                bool exchange_success = false;
                if (tail_node == result)
                {
                    // If tail and head are same we try to set the tail to nullptr.
                    //   This indicates during a push that the head needs to be set again.
                    exchange_success = std::atomic_compare_exchange_strong_explicit(
                        &queue._tail,
                        &tail_node,
                        nullptr,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed
                    );
                }

                // If we failed the exchange, this means that tail moved forward
                if (exchange_success == false)
                {
                    while (result->next == nullptr)
                    {
                        std::atomic_thread_fence(std::memory_order_acquire);
                    }

                    NodeType* previous = std::atomic_exchange_explicit(
                        &queue._head, result->next, std::memory_order_relaxed
                    );
                    ICE_ASSERT_CORE(previous == nullptr);
                }
            }
            return result;
        }

        template<typename NodeType>
        inline auto consume(ice::AtomicLinkedQueue<NodeType>& queue) noexcept -> ice::LinkedQueueRange<NodeType>
        {
            ice::LinkedQueueRange<NodeType> result{ ._head = nullptr, ._tail = nullptr };

            NodeType* const head_result = std::atomic_exchange_explicit(
                &queue._head, nullptr, std::memory_order_relaxed
            );

            if (head_result != nullptr)
            {
                result._head = head_result;
                result._tail = std::atomic_exchange_explicit(
                    &queue._tail, nullptr, std::memory_order_acquire
                );
                ICE_ASSERT_CORE(result._tail != nullptr);
            }

            return result;
        }

    } // linked_queue

} // namespace ice
