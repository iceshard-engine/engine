#include "collections.hxx"

// core::pod::Array members
//////////////////////////////////////////////////////////////////////////

template<typename T>
inline core::pod::Array<T>::Array(core::allocator &alloc) noexcept
    : _allocator{ &alloc }
{
}

template<typename T>
inline core::pod::Array<T>::Array(const Array<T> &other) noexcept
    : _allocator{ other._allocator }
{
    const auto other_size = other._size;

    array::set_capacity(*this, other_size);
    memcpy(_data, other._data, sizeof(T) * other_size);
    _size = other_size;
}

template<typename T>
inline core::pod::Array<T>::Array(Array<T> &&other) noexcept
    : _allocator{ other._allocator }
    , _data{ std::exchange(other._data, nullptr) }
    , _size{ std::exchange(other._size, 0) }
    , _capacity{ std::exchange(other._capacity, 0) }
{
}

template<typename T>
inline core::pod::Array<T>::~Array() noexcept
{
    _allocator->deallocate(_data);
}

template<typename T>
auto core::pod::Array<T>::operator=(const Array<T> &other) noexcept -> Array<T> &
{
    const auto other_size = other._size;

    array::resize(*this, other_size);
    memcpy(_data, other._data, sizeof(T) * other_size);
    return *this;
}

template<typename T>
inline auto core::pod::Array<T>::operator=(Array && other) noexcept -> Array &
{
    if (this != &other)
    {
        IS_ASSERT(_allocator == other._allocator, "Allocators differ on array objects!");
        std::swap(_data, other._data);
        _capacity = std::exchange(other._capacity, 0);
        _size = std::exchange(other._size, 0);
    }
    return *this;
}

template<typename T>
inline auto core::pod::Array<T>::operator[](uint32_t i) -> T &
{
    return _data[i];
}

template<typename T>
inline auto core::pod::Array<T>::operator[](uint32_t i) const -> const T &
{
    return _data[i];
}

template<typename T>
inline auto core::pod::begin(Array<T> &a) noexcept -> T *
{
    return array::begin(a);
}

template<typename T>
inline auto core::pod::begin(const Array<T> &a) noexcept -> const T *
{
    return array::begin(a);
}

template<typename T>
inline auto core::pod::end(Array<T> &a) noexcept -> T *
{
    return array::end(a);
}

template<typename T>
inline auto core::pod::end(const Array<T> &a) noexcept -> const T *
{
    return array::end(a);
}

template<typename T>
inline void core::pod::swap(Array<T> &lhs, Array<T> &rhs) noexcept
{
    std::swap(lhs._allocator, rhs._allocator);
    std::swap(lhs._size, rhs._size);
    std::swap(lhs._capacity, rhs._capacity);
    std::swap(lhs._data, rhs._data);
}

// core::pod::Array free functions
//////////////////////////////////////////////////////////////////////////

template<typename T>
inline auto core::pod::array::size(const Array<T> &a) noexcept -> uint32_t
{
    return a._size;
}

template<typename T>
inline bool core::pod::array::any(const Array<T> &a) noexcept
{
    return a._size != 0;
}

template<typename T>
inline bool core::pod::array::empty(const Array<T> &a) noexcept
{
    return a._size == 0;
}

template<typename T>
inline auto core::pod::array::begin(Array<T> &a) noexcept -> T *
{
    return a._data;
}

template<typename T>
inline auto core::pod::array::begin(const Array<T> &a) noexcept -> const T *
{
    return a._data;
}

template<typename T>
inline auto core::pod::array::end(Array<T> &a) noexcept -> T *
{
    return a._data + a._size;
}

template<typename T>
inline auto core::pod::array::end(const Array<T> &a) noexcept -> const T *
{
    return a._data + a._size;
}

template<typename T>
inline auto core::pod::array::front(Array<T> &a) noexcept -> T &
{
    return a._data[0];
}

template<typename T>
inline auto core::pod::array::front(const Array<T> &a) noexcept -> const T &
{
    return a._data[0];
}

template<typename T>
inline auto core::pod::array::back(Array<T> &a) noexcept -> T &
{
    return a._data[a._size - 1];
}

template<typename T>
inline auto core::pod::array::back(const Array<T> &a) noexcept -> const T &
{
    return a._data[a._size - 1];
}

template<typename T>
inline void core::pod::array::clear(Array<T> &a) noexcept
{
    resize(a, 0);
}

template<typename T>
inline void core::pod::array::trim(Array<T> &a) noexcept
{
    set_capacity(a, a._size);
}

template<typename T>
inline void core::pod::array::resize(Array<T> &a, uint32_t new_size) noexcept
{
    if (new_size > a._capacity)
    {
        grow(a, new_size);
    }
    a._size = new_size;
}

template<typename T>
inline void core::pod::array::reserve(Array<T> &a, uint32_t new_capacity) noexcept
{
    if (new_capacity > a._capacity)
        set_capacity(a, new_capacity);
}

template<typename T>
inline void core::pod::array::set_capacity(Array<T> &a, uint32_t new_capacity) noexcept
{
    if (new_capacity == a._capacity)
    {
        return;
    }

    if (new_capacity < a._size)
    {
        resize(a, new_capacity);
    }

    T *new_data = 0;
    if (new_capacity > 0)
    {
        new_data = (T *)a._allocator->allocate(sizeof(T) * new_capacity, alignof(T));
        memcpy(new_data, a._data, sizeof(T) * a._size);
    }

    a._allocator->deallocate(a._data);
    a._data = new_data;
    a._capacity = new_capacity;
}

template<typename T>
inline void core::pod::array::grow(Array<T> &a, uint32_t min_capacity) noexcept
{
    uint32_t new_capacity = a._capacity * 2 + 8;
    if (new_capacity < min_capacity)
    {
        new_capacity = min_capacity;
    }
    set_capacity(a, new_capacity);
}

template<typename T>
inline void core::pod::array::push_back(Array<T> &a, const T &item) noexcept
{
    if (a._size + 1 > a._capacity)
    {
        grow(a);
    }
    a._data[a._size++] = item;
}

template<typename T>
inline void core::pod::array::pop_back(Array<T> &a) noexcept
{
    a._size--;
}
