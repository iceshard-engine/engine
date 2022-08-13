#pragma once
#include <ice/pod/collections.hxx>
#include <ice/pod/array.hxx>

namespace ice::pod
{

    namespace queue
    {

        template<typename T>
        inline void reserve(ice::pod::Queue<T>& queue, uint32_t size) noexcept;

        template<typename T>
        inline void push_back(ice::pod::Queue<T>& queue, T const& item) noexcept;

        template<typename T>
        inline void push_back(ice::pod::Queue<T>& queue, ice::Span<T const> items) noexcept;

        template<typename T>
        inline void pop_back(ice::pod::Queue<T>& queue) noexcept;

        template<typename T>
        inline void push_front(ice::pod::Queue<T>& queue, T const& item) noexcept;

        template<typename T>
        inline void pop_front(ice::pod::Queue<T>& queue) noexcept;

        template<typename T>
        inline void consume(ice::pod::Queue<T>& queue, uint32_t count) noexcept;


        template<typename T>
        inline auto size(ice::pod::Queue<T> const& queue) noexcept -> uint32_t;

        template<typename T>
        inline auto space(ice::pod::Queue<T> const& queue) noexcept -> uint32_t;

        template<typename T>
        inline auto front(ice::pod::Queue<T> const& queue) noexcept -> T const&;

        template<typename T>
        inline auto back(ice::pod::Queue<T> const& queue) noexcept -> T const&;

        template<typename T, typename Fn>
        inline void for_each(ice::pod::Queue<T> const& queue, Fn&& fn) noexcept;

    } // namespace queue

    template<typename T>
    inline Queue<T>::Queue(ice::Allocator& alloc) noexcept
        : _data{ alloc }
    {
    }

    template<typename T>
    inline auto Queue<T>::operator[](uint32_t idx) noexcept -> T&
    {
        return _data[(idx + _offset) % ice::pod::array::size(_data)];
    }

    template<typename T>
    inline auto Queue<T>::operator[](uint32_t idx) const noexcept -> T const&
    {
        return _data[(idx + _offset) % ice::pod::array::size(_data)];
    }

    namespace detail::queue
    {

        template<typename T>
        void increase_capacity(ice::pod::Queue<T>& queue, uint32_t new_capacity) noexcept
        {
            uint32_t const old_size = array::size(queue._data);
            ice::pod::array::resize(queue._data, new_capacity);

            if (queue._offset + queue._size > old_size)
            {
                uint32_t end_items = old_size - queue._offset;
                std::memmove(
                    ice::pod::array::begin(queue._data) + new_capacity - end_items,
                    ice::pod::array::begin(queue._data) + queue._offset,
                    end_items * sizeof(T)
                );
                queue._offset += new_capacity - old_size;
            }
        }

        template<typename T>
        void grow(ice::pod::Queue<T>& queue, uint32_t min_capacity = 0) noexcept
        {
            uint32_t new_capacity = array::size(queue._data) * 2 + 8;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            detail::queue::increase_capacity(queue, new_capacity);
        }

    } // namespace detail::queue

    namespace queue
    {

        template<typename T>
        inline void reserve(ice::pod::Queue<T>& queue, uint32_t capacity) noexcept
        {
            if (capacity > ice::pod::array::size(queue._data))
            {
                ice::pod::detail::queue::increase_capacity(queue, capacity);
            }
        }

        template<typename T>
        inline void push_back(ice::pod::Queue<T>& queue, T const& item) noexcept
        {
            if (ice::pod::queue::space(queue) == 0)
            {
                ice::pod::detail::queue::grow(queue);
            }

            queue[queue._size] = item;
            queue._size += 1;
        }

        template<typename T>
        inline void push_back(ice::pod::Queue<T>& queue, ice::Span<T const> items) noexcept
        {
            if (ice::pod::queue::space(queue) < items.size())
            {
                ice::pod::detail::queue::grow(queue, static_cast<uint32_t>(items.size()));
            }

            uint32_t const data_size = ice::pod::array::size(queue._data);

            T const* source_data = items.data();
            uint32_t insert_pos = (queue._offset + queue._size) % data_size;
            uint32_t insert_count = static_cast<uint32_t>(items.size());

            if (insert_pos + insert_count > data_size)
            {
                uint32_t const fill_count = data_size - insert_pos;

                ice::memcpy(ice::pod::array::begin(queue._data) + insert_pos, source_data, fill_count * sizeof(T));
                queue._size += fill_count;
                source_data += fill_count;
                insert_count -= fill_count;
                insert_pos = 0;
            }

            ice::memcpy(ice::pod::array::begin(queue._data) + insert_pos, source_data, insert_count * sizeof(T));
            queue._size += insert_count;
        }

        template<typename T>
        inline void pop_back(ice::pod::Queue<T>& queue) noexcept
        {
            queue._size -= 1;
        }

        template<typename T>
        inline void push_front(ice::pod::Queue<T>& queue, T const& item) noexcept
        {
            if (ice::pod::queue::space(queue) == 0)
            {
                ice::pod::detail::queue::grow(queue);
            }

            queue._offset = queue._offset - 1 + ice::pod::array::size(queue._data);
            queue._offset %= ice::pod::array::size(queue._data);
            queue._size += 1;
            queue[0] = item;
        }

        template<typename T>
        inline void pop_front(ice::pod::Queue<T>& queue) noexcept
        {
            queue._offset = (queue._offset + 1) % ice::pod::array::size(queue._data);
            queue._size -= 1;
        }

        template<typename T>
        inline void consume(ice::pod::Queue<T>& queue, uint32_t count) noexcept
        {
            queue._offset = (queue._offset + count) % ice::pod::array::size(queue._data);
            queue._size -= count;
        }


        template<typename T>
        inline auto size(ice::pod::Queue<T> const& queue) noexcept -> uint32_t
        {
            return queue._size;
        }

        template<typename T>
        inline auto space(ice::pod::Queue<T> const& queue) noexcept -> uint32_t
        {
            return ice::pod::array::size(queue._data) - queue._size;
        }

        template<typename T>
        inline auto front(ice::pod::Queue<T> const& queue) noexcept -> T const&
        {
            return queue[0];
        }

        template<typename T>
        inline auto back(ice::pod::Queue<T> const& queue) noexcept -> T const&
        {
            return queue[queue._size - 1];
        }

        template<typename T, typename Fn>
        inline void for_each(ice::pod::Queue<T> const& queue, Fn&& fn) noexcept
        {
            uint32_t const data_size = ice::pod::array::size(queue._data) - 1;
            uint32_t const begin = queue._offset;

            if (begin + queue._size <= data_size)
            {
                uint32_t const end = begin + queue._size;

                for (uint32_t idx = begin; idx < end; ++idx)
                {
                    ice::forward<Fn>(fn)(queue._data[idx]);
                }
            }
            else
            {
                uint32_t const end_wrapped = (begin + queue._size) - data_size;

                for (uint32_t idx = begin; idx < data_size; ++idx)
                {
                    ice::forward<Fn>(fn)(queue._data[idx]);
                }
                for (uint32_t idx = 0; idx < end_wrapped; ++idx)
                {
                    ice::forward<Fn>(fn)(queue._data[idx]);
                }
            }
        }

    } // namespace queue

} // namespace ice::pod

