/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/container_concepts.hxx>
#include <atomic>

namespace ice
{

    template<ice::concepts::LinkedListNode NodeType>
    struct AtomicLinkedQueueRange;

    template<ice::concepts::LinkedListNode NodeType>
    struct AtomicLinkedQueue
    {
        using ValueType = NodeType;

        std::atomic<NodeType*> _head;
        std::atomic<NodeType*> _tail;

        constexpr AtomicLinkedQueue() noexcept;
        constexpr ~AtomicLinkedQueue() noexcept = default;

        constexpr AtomicLinkedQueue(AtomicLinkedQueue&& other) noexcept;
        constexpr AtomicLinkedQueue(AtomicLinkedQueue const& other) noexcept = delete;

        constexpr auto operator=(AtomicLinkedQueue&& other) noexcept -> AtomicLinkedQueue&;
        constexpr auto operator=(AtomicLinkedQueue const& other) noexcept -> AtomicLinkedQueue& = delete;

        constexpr bool is_empty() const noexcept { return _head.load(std::memory_order_relaxed) == nullptr; }
        constexpr bool not_empty() const noexcept { return is_empty() == false; }

        template<ice::concepts::LinkedListNode DerivedNodeType = NodeType>
        constexpr void push_back(DerivedNodeType* node) noexcept;
        template<ice::concepts::LinkedListNode DerivedNodeType = NodeType>
        constexpr bool push_back(ice::AtomicLinkedQueueRange<DerivedNodeType> range) noexcept;

        [[nodiscard]]
        constexpr auto take_front() noexcept -> NodeType*;
        [[nodiscard]]
        constexpr auto take_all() noexcept -> ice::AtomicLinkedQueueRange<NodeType>;
    };

    template<ice::concepts::LinkedListNode NodeType>
    struct AtomicLinkedQueueRange
    {
        using ValueType = NodeType;

        NodeType* _head = nullptr;
        NodeType* _tail = nullptr;

        struct Iterator
        {
            NodeType* _current;
            NodeType* _next;
            NodeType* _tail;

            constexpr auto operator*() const noexcept -> NodeType*;
            constexpr void operator++() noexcept;

            constexpr bool operator==(Iterator other) const noexcept;
            constexpr bool operator!=(Iterator other) const noexcept;
        };

        constexpr auto begin() const noexcept -> Iterator;
        constexpr auto end() const noexcept -> Iterator;

        constexpr bool is_empty() const noexcept { return _head == nullptr; }
        constexpr bool not_empty() const noexcept { return is_empty() == false; }
    };

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr AtomicLinkedQueue<NodeType>::AtomicLinkedQueue() noexcept
        : _head{ }
        , _tail{ }
    {
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr AtomicLinkedQueue<NodeType>::AtomicLinkedQueue(AtomicLinkedQueue&& other) noexcept
        : _head{ }
        , _tail{ }
    {
        // We move atomically to this object
        ice::AtomicLinkedQueueRange<NodeType> range = other.take_all();
        _head = range._head;
        _tail = range._tail;
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueue<NodeType>::operator=(AtomicLinkedQueue&& other) noexcept -> AtomicLinkedQueue&
    {
        if (this != &other)
        {
            // We move atomically to this object
            ice::AtomicLinkedQueueRange<NodeType> range = other.take_all();
            _head = range._head;
            _tail = range._tail;
        }
        return *this;
    }

    template<ice::concepts::LinkedListNode NodeType>
    template<ice::concepts::LinkedListNode DerivedNodeType>
    inline constexpr void AtomicLinkedQueue<NodeType>::push_back(DerivedNodeType* node) noexcept
    {
        NodeType* const previous_tail = _tail.exchange(node, std::memory_order_relaxed);

        if (previous_tail == nullptr)
        {
            _head.store(node, std::memory_order_relaxed);
        }
        else
        {
            previous_tail->_next = node;
        }

        std::atomic_thread_fence(std::memory_order_release);
    }

    template<ice::concepts::LinkedListNode NodeType>
    template<ice::concepts::LinkedListNode DerivedNodeType>
    inline constexpr bool AtomicLinkedQueue<NodeType>::push_back(
        ice::AtomicLinkedQueueRange<DerivedNodeType> range
    ) noexcept
    {
        // If no TAIL, then the range is empty
        if (range._tail == nullptr)
        {
            return false;
        }

        // If we have a TAIL we need to also have a HEAD.
        ICE_ASSERT_CORE(range._head != nullptr);

        // Appending a range is a simple as adding one node.
        // - We take the tail of the range and push it as the new tail on the queue
        // - We set the previous_tail->next pointer to the pushed range head or set is as the new head.
        // - All other values are still intact and the operation is still atomic like it was in the single node case.

        NodeType* const previous_tail = _tail.exchange(range._tail, std::memory_order_relaxed);

        if (previous_tail == nullptr)
        {
            _head.store(range._head, std::memory_order_relaxed);
        }
        else
        {
            previous_tail->_next = range._head;
        }

        std::atomic_thread_fence(std::memory_order_release);
        return true;
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueue<NodeType>::take_front() noexcept -> NodeType*
    {
        NodeType* volatile result_node = _head.exchange(nullptr, std::memory_order_relaxed);

        if (result_node == nullptr)
        {
            // TODO: Consider if we should spin 100 tries to get something if tail != nullptr.
            //   or let the caller always handle this case?
        }

        if (result_node != nullptr)
        {
            NodeType* tail_node = _tail.load(std::memory_order_relaxed);

            // We only require a tail update if the result node (head) is different from the tail.
            bool skip_tail_update = tail_node == result_node;
            if (skip_tail_update)
            {
                // If tail and head are same we try to set the tail to nullptr.
                //   This indicates during a push that the head needs to be set again.
                skip_tail_update = _tail.compare_exchange_strong(
                    tail_node,
                    nullptr,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                );
            }

            // If we failed the exchange, this means that a tail exists.
            if (skip_tail_update == false)
            {
                // It might happen that the 'result_node' (former head) has it's '_next' pointer not yet set,
                //  requiring us to wait for the new 'head' pointer to be set from another thread.
                NodeType volatile* next = result_node;
                while (next->_next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                NodeType* previous = _head.exchange(next->_next, std::memory_order_relaxed);
                // A '_head' pointer should never have value set during a 'take_front' operation.
                ICE_ASSERT_CORE(previous == nullptr);
            }
        }
        return result_node;
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueue<NodeType>::take_all() noexcept -> ice::AtomicLinkedQueueRange<NodeType>
    {
        ice::AtomicLinkedQueueRange<NodeType> result{ ._head = nullptr, ._tail = nullptr };

        NodeType* const head_result = _head.exchange(nullptr, std::memory_order_relaxed);

        // If a value existed at the '_head' that means when we take the current '_tail'
        //   all values are explicitly accessible only to us!
        if (head_result != nullptr)
        {
            result._head = head_result;
            result._tail = _tail.exchange(nullptr, std::memory_order_acquire);
            ICE_ASSERT_CORE(result._tail != nullptr);
        }

        return result;
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueueRange<NodeType>::Iterator::operator*() const noexcept -> NodeType*
    {
        return _current;
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr void AtomicLinkedQueueRange<NodeType>::Iterator::operator++() noexcept
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
                while (next->_next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                _next = next->_next;
            }
        }
        else
        {
            ICE_ASSERT_CORE(_current == _tail);
            _current = nullptr;
        }
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr bool AtomicLinkedQueueRange<NodeType>::Iterator::operator==(Iterator other) const noexcept
    {
        return (_tail == other._tail) && (_current == other._current);
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr bool AtomicLinkedQueueRange<NodeType>::Iterator::operator!=(Iterator other) const noexcept
    {
        return !(*this == other);
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueueRange<NodeType>::begin() const noexcept -> Iterator
    {
        if (_head != nullptr)
        {
            if (_head == _tail)
            {
                return { _head, nullptr, _tail };
            }

            // NOTE: Wait for the first 'next' pointer to be set from a different thread.
            NodeType volatile* next = _head;
            while (next->_next == nullptr)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
            }

            return { _head, next->_next, _tail };
        }
        else
        {
            return end();
        }
    }

    template<ice::concepts::LinkedListNode NodeType>
    inline constexpr auto AtomicLinkedQueueRange<NodeType>::end() const noexcept -> Iterator
    {
        return { nullptr, nullptr, _tail };
    }

} // namespace ice
