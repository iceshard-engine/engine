#pragma once
#include <core/pod/collections.hxx>
#include <core/pod/array.hxx>

namespace core::pod
{

    //! \brief Contains functions used to modify Queue<T> objects.
    namespace queue
    {

        //! \brief Returns the number of items in the queue.
        template<typename T>
        auto size(Queue<T> const& q) noexcept -> uint32_t;

        //! \brief Returns the amount of free space in the queue/ring buffer.
        //! \details This is the number of items we can push before the queue needs to grow.
        template<typename T>
        auto space(Queue<T> const& q) noexcept -> uint32_t;

        //! \brief Ensures the queue has room for at least the specified number of items.
        template<typename T>
        void reserve(Queue<T>& q, uint32_t size) noexcept;

        //! \brief Pushes the item to the end of the queue.
        template<typename T>
        void push_back(Queue<T>& q, T const& item) noexcept;

        //! \brief Pops the last item from the queue. The queue cannot be empty.
        template<typename T>
        void pop_back(Queue<T>& q) noexcept;

        //! \brief Pushes the item to the front of the queue.
        template<typename T>
        void push_front(Queue<T>& q, T const& item) noexcept;

        //! \brief Pops the first item from the queue. The queue cannot be empty.
        template<typename T>
        void pop_front(Queue<T>& q) noexcept;

        //! \brief Consumes n items from the front of the queue.
        template<typename T>
        void consume(Queue<T>& q, uint32_t n) noexcept;

        //! \brief Pushes n items to the back of the queue.
        template<typename T>
        void push(Queue<T>& q, T const* items, uint32_t n) noexcept;

        //! \brief Pushes the array at the back of the queue.
        template<typename T, uint32_t Size>
        void push(Queue<T>& q, T const(&arr)[Size]) noexcept;

        //! \brief Returns the begin and end of the continuous chunk of elements at
        //!     the start of the queue.
        //!
        //! \details This can be useful for when you want to process many queue elements at
        //!     once.
        //!
        //! \remarks This chunk does not necessarily contain all the elements
        //!     in the queue (if the queue wraps around the array).
        template<typename T>
        auto begin_front(Queue<T>& q) noexcept -> T*;

        template<typename T>
        auto begin_front(Queue<T> const& q) noexcept -> T const*;

        template<typename T>
        auto end_front(Queue<T>& q) noexcept -> T*;

        template<typename T>
        auto end_front(Queue<T> const& q) noexcept -> T const*;

        template<typename T, typename Fn>
        void for_each(Queue<T> const& q, Fn&& fn) noexcept;

    } // namespace queue

} // namespace core::pod

#include "queue.inl"
