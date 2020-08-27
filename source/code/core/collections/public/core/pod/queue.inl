
template<typename T>
inline core::pod::Queue<T>::Queue(core::allocator& allocator) noexcept
    : _data{ allocator }
{
}

template<typename T>
inline auto core::pod::Queue<T>::operator[](uint32_t i) -> T &
{
    return _data[(i + _offset) % array::size(_data)];
}

template<typename T>
inline auto core::pod::Queue<T>::operator[](uint32_t i) const -> T const&
{
    return _data[(i + _offset) % array::size(_data)];
}

// core::pod::Queue internal functions
//////////////////////////////////////////////////////////////////////////

namespace core::pod::queue_internal
{

    // Can only be used to increase the capacity.
    template<typename T>
    void increase_capacity(Queue<T>& q, uint32_t new_capacity) noexcept
    {
        uint32_t const array_size = array::size(q._data);
        array::resize(q._data, new_capacity);

        if (q._offset + q._size > array_size)
        {
            uint32_t end_items = array_size - q._offset;
            memmove(array::begin(q._data) + new_capacity - end_items, array::begin(q._data) + q._offset, end_items * sizeof(T));
            q._offset += new_capacity - array_size;
        }
    }

    template<typename T>
    void grow(Queue<T>& q, uint32_t min_capacity = 0) noexcept
    {
        uint32_t new_capacity = array::size(q._data) * 2 + 8;
        if (new_capacity < min_capacity)
        {
            new_capacity = min_capacity;
        }
        increase_capacity(q, new_capacity);
    }

} // namespace core::pod::queue_internal

// core::pod::Queue free functions
//////////////////////////////////////////////////////////////////////////

template<typename T>
inline auto core::pod::queue::size(Queue<T> const& q) noexcept -> uint32_t
{
    return q._size;
}

template<typename T>
inline auto core::pod::queue::space(Queue<T> const& q) noexcept -> uint32_t
{
    return array::size(q._data) - q._size;
}

template<typename T>
inline void core::pod::queue::reserve(Queue<T>& q, uint32_t size) noexcept
{
    if (size > q._size)
    {
        queue_internal::increase_capacity(q, size);
    }
}

template<typename T>
inline void core::pod::queue::push_back(Queue<T>& q, T const& item) noexcept
{
    if (space(q) == 0)
    {
        queue_internal::grow(q);
    }

    q[q._size++] = item;
}

template<typename T>
inline void core::pod::queue::pop_back(Queue<T>& q) noexcept
{
    --q._size;
}

template<typename T>
inline void core::pod::queue::push_front(Queue<T>& q, T const& item) noexcept
{
    if (!space(q))
    {
        queue_internal::grow(q);
    }

    q._offset = (q._offset - 1 + array::size(q._data)) % array::size(q._data);
    ++q._size;
    q[0] = item;
}

template<typename T>
inline void core::pod::queue::pop_front(Queue<T>& q) noexcept
{
    q._offset = (q._offset + 1) % array::size(q._data);
    --q._size;
}

template<typename T>
inline void core::pod::queue::consume(Queue<T>& q, uint32_t n) noexcept
{
    q._offset = (q._offset + n) % array::size(q._data);
    q._size -= n;
}

template<typename T>
inline void core::pod::queue::push(Queue<T>& q, T const* items, uint32_t n) noexcept
{
    if (space(q) < n)
    {
        queue_internal::grow(q, size(q) + n);
    }

    uint32_t const size = array::size(q._data);
    uint32_t const insert = (q._offset + q._size) % size;
    uint32_t to_insert = n;
    if (insert + to_insert > size)
    {
        to_insert = size - insert;
    }

    memcpy(array::begin(q._data) + insert, items, to_insert * sizeof(T));
    q._size += to_insert;
    items += to_insert;
    n -= to_insert;
    memcpy(array::begin(q._data), items, n * sizeof(T));
    q._size += n;
}

template<typename T, uint32_t Size>
inline void core::pod::queue::push(Queue<T>& q, T const(&arr)[Size]) noexcept
{
    push(q, &arr[0], Size);
}

template<typename T>
inline auto core::pod::queue::begin_front(Queue<T>& q) noexcept -> T *
{
    return array::begin(q._data) + q._offset;
}

template<typename T>
inline auto core::pod::queue::begin_front(Queue<T> const& q) noexcept -> T const*
{
    return array::begin(q._data) + q._offset;
}

template<typename T>
inline auto core::pod::queue::end_front(Queue<T>& q) noexcept -> T *
{
    uint32_t end = q._offset + q._size;
    return end > array::size(q._data) ? array::end(q._data) : array::begin(q._data) + end;
}

template<typename T>
inline auto core::pod::queue::end_front(Queue<T> const& q) noexcept -> T const*
{
    uint32_t end = q._offset + q._size;
    return end > array::size(q._data) ? array::end(q._data) : array::begin(q._data) + end;
}

template<typename T, typename Fn>
inline void core::pod::queue::for_each(Queue<T> const& q, Fn&& fn) noexcept
{
    uint32_t const data_size = array::size(q._data) - 1;
    uint32_t const begin = q._offset;

    if (begin + q._size <= data_size)
    {
        uint32_t const end = begin + q._size;

        for (auto it = begin; it < end; ++it)
        {
            std::forward<Fn>(fn)(q._data[it]);
        }
    }
    else
    {
        uint32_t const end_wrapped = (begin + q._size) - data_size;

        for (auto it = begin; it < data_size; ++it)
        {
            std::forward<Fn>(fn)(q._data[it]);
        }
        for (auto it = 0; it < end_wrapped; ++it)
        {
            std::forward<Fn>(fn)(q._data[it]);
        }
    }
}
