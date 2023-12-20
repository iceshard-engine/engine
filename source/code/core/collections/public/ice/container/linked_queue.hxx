/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_concepts.hxx>
#include <atomic>

namespace ice
{

    template<typename NodeType>
        requires ice::LinkedListNode<NodeType>
    struct AtomicLinkedQueue
    {
        using ValueType = NodeType;

        // TODO: BENCHMARK, Then add enough space between the atomic values to avoid false sharing. BENCHMARK AGAIN!
        std::atomic<NodeType*> _head;
        std::atomic<NodeType*> _tail;

        inline AtomicLinkedQueue() noexcept;
        inline ~AtomicLinkedQueue() noexcept = default;

        inline AtomicLinkedQueue(AtomicLinkedQueue&& other) noexcept;
        inline AtomicLinkedQueue(AtomicLinkedQueue const& other) noexcept = delete;

        inline auto operator=(AtomicLinkedQueue&& other) noexcept -> AtomicLinkedQueue&;
        inline auto operator=(AtomicLinkedQueue const& other) noexcept -> AtomicLinkedQueue & = delete;
    };

    template<typename NodeType>
        requires ice::LinkedListNode<NodeType>
    struct LinkedQueueRange
    {
        using ValueType = NodeType;

        NodeType* _head;
        NodeType* _tail;

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
    };

    namespace linked_queue
    {

        template<typename NodeType>
        constexpr bool any(ice::LinkedQueueRange<NodeType> const& queue_range) noexcept;

        template<typename NodeType>
        constexpr bool empty(ice::LinkedQueueRange<NodeType> const& queue_range) noexcept;

        template<typename NodeType>
        constexpr auto begin(
            ice::LinkedQueueRange<NodeType> const& queue_range
        ) noexcept -> typename ice::LinkedQueueRange<NodeType>::Iterator;

        template<typename NodeType>
        constexpr auto end(
            ice::LinkedQueueRange<NodeType> const& queue_range
        ) noexcept -> typename ice::LinkedQueueRange<NodeType>::Iterator;


        template<typename NodeType>
        inline bool any(ice::AtomicLinkedQueue<NodeType> const& queue) noexcept;

        template<typename NodeType>
        inline bool empty(ice::AtomicLinkedQueue<NodeType> const& queue) noexcept;

        template<typename NodeType, typename DerivedNodeType = NodeType>
        inline void push(ice::AtomicLinkedQueue<NodeType>& queue, DerivedNodeType* node) noexcept;

        template<typename NodeType>
        inline auto pop(ice::AtomicLinkedQueue<NodeType>& queue) noexcept -> NodeType*;

        template<typename NodeType>
        inline auto consume(ice::AtomicLinkedQueue<NodeType>& queue) noexcept -> ice::LinkedQueueRange<NodeType>;

    } // namespace linked_queue

} // namespace ice

namespace ice
{

    using ice::linked_queue::begin;
    using ice::linked_queue::end;

} // namespace ice

#include "impl/linked_queue_impl.inl"
