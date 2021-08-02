#pragma once
#include <ice/pod/collections.hxx>

namespace ice::pod
{

    namespace array
    {

        template<typename T>
        inline void set_capacity(ice::pod::Array<T>& arr, uint32_t new_capacity) noexcept;

        template<typename T>
        inline void reserve(ice::pod::Array<T>& arr, uint32_t min_capacity) noexcept;

        template<typename T>
        inline void grow(ice::pod::Array<T>& arr, uint32_t min_capacity = 0) noexcept;

        template<typename T>
        inline void resize(ice::pod::Array<T>& arr, uint32_t new_size) noexcept;

        template<typename T>
        inline void shrink(ice::pod::Array<T>& arr) noexcept;

        template<typename T>
        inline void clear(ice::pod::Array<T>& arr) noexcept;

        template<typename T>
        inline auto span(ice::pod::Array<T>& arr, ice::u32 offset = 0, ice::u32 count = ~0u) noexcept -> ice::Span<T>;

        template<typename T, typename U = T>
        inline void push_back(ice::pod::Array<T>& arr, U const& item) noexcept;

        template<typename T>
        inline void push_back(ice::pod::Array<T>& arr, ice::pod::Array<T> const& items) noexcept;

        template<typename T>
        inline void push_back(ice::pod::Array<T>& arr, ice::Span<T const> items) noexcept;

        template<typename T>
        inline void pop_back(ice::pod::Array<T>& arr, uint32_t count = 1) noexcept;

        template<typename T>
        inline auto begin(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::Iterator;

        template<typename T>
        inline auto end(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::Iterator;

        template<typename T>
        inline auto rbegin(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::ReverseIterator;

        template<typename T>
        inline auto rend(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::ReverseIterator;


        template<typename T>
        inline auto size(ice::pod::Array<T> const& arr) noexcept -> uint32_t;

        template<typename T>
        inline auto capacity(ice::pod::Array<T> const& arr) noexcept -> uint32_t;

        template<typename T>
        inline bool any(ice::pod::Array<T> const& arr) noexcept;

        template<typename T>
        inline bool empty(ice::pod::Array<T> const& arr) noexcept;

        template<typename T>
        inline auto span(ice::pod::Array<T> const& arr, ice::u32 offset = 0, ice::u32 count = ~0u) noexcept -> ice::Span<T const>;

        template<typename T>
        inline auto begin(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstIterator;

        template<typename T>
        inline auto end(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstIterator;

        template<typename T>
        inline auto rbegin(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstReverseIterator;

        template<typename T>
        inline auto rend(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstReverseIterator;

        template<typename T>
        inline auto front(ice::pod::Array<T> const& arr) noexcept -> T const&;

        template<typename T>
        inline auto back(ice::pod::Array<T> const& arr) noexcept -> T const&;

    } // namespace array

    template<typename T>
    inline Array<T>::Array(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
    { }

    template<typename T>
    inline Array<T>::Array(Array&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ other._size }
        , _capacity{ other._capacity }
        , _data{ other._data }
    {
        other._size = 0;
        other._capacity = 0;
        other._data = nullptr;
    }

    template<typename T>
    inline Array<T>::Array(Array const& other) noexcept
        : _allocator{ other._allocator }
    {
        if (other._size > 0)
        {
            ice::pod::array::set_capacity(*this, other._capacity);
            ice::memcpy(_data, other._data, sizeof(T) * other._size);
            _size = other._size;
        }
    }

    template<typename T>
    inline Array<T>::~Array() noexcept
    {
        _allocator->deallocate(_data);
    }

    template<typename T>
    inline auto Array<T>::operator=(Array&& other) noexcept -> Array&
    {
        if (this != &other)
        {
            _allocator->destroy(_data);

            _allocator = other._allocator;
            _capacity = ice::exchange(other._capacity, 0);
            _data = ice::exchange(other._data, nullptr);
            _size = ice::exchange(other._size, 0);
        }
        return *this;
    }

    template<typename T>
    inline auto Array<T>::operator=(Array const& other) noexcept -> Array&
    {
        if (this != &other)
        {
            ice::pod::array::reserve(*this, other._capacity);
            if (other._size > 0)
            {
                ice::memcpy(_data, other._data, sizeof(T) * other._size);
            }
            _size = other._size;
        }
        return *this;
    }

    template<typename T>
    inline auto Array<T>::operator[](uint32_t i) noexcept -> T&
    {
        // #todo assert
        return _data[i];
    }

    template<typename T>
    inline auto Array<T>::operator[](uint32_t i) const noexcept -> T const&
    {
        // #todo assert
        return _data[i];
    }

    template<typename T>
    inline Array<T>::operator ice::Span<T>() noexcept
    {
        return { _data, _size };
    }

    template<typename T>
    inline Array<T>::operator ice::Span<T const>() const noexcept
    {
        return { _data, _size };
    }

    namespace array
    {

        template<typename T>
        inline void set_capacity(ice::pod::Array<T>& arr, uint32_t new_capacity) noexcept
        {
            if (new_capacity == arr._capacity)
            {
                return;
            }

            if (new_capacity < arr._size)
            {
                arr._size = new_capacity;
            }

            T* new_data = nullptr;
            if (new_capacity > 0)
            {
                uint32_t const alignment = ice::max(static_cast<uint32_t>(alignof(T)), ice::Allocator::Constant_DefaultAlignment);

                void* new_buffer = arr._allocator->allocate(sizeof(T) * new_capacity, alignment);
                if (arr._size > 0)
                {
                    ice::memcpy(new_buffer, arr._data, sizeof(T) * arr._size);
                }
                new_data = reinterpret_cast<T*>(new_buffer);
            }

            arr._allocator->deallocate(arr._data);
            arr._data = new_data;
            arr._capacity = new_capacity;
        }

        template<typename T>
        inline void reserve(ice::pod::Array<T>& arr, uint32_t min_capacity) noexcept
        {
            if (arr._capacity < min_capacity)
            {
                ice::pod::array::set_capacity(arr, min_capacity);
            }
        }

        template<typename T>
        inline void grow(ice::pod::Array<T>& arr, uint32_t min_capacity) noexcept
        {
            uint32_t new_capacity = arr._capacity * 2 + 8;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            ice::pod::array::set_capacity(arr, new_capacity);
        }

        template<typename T>
        inline void resize(ice::pod::Array<T>& arr, uint32_t new_size) noexcept
        {
            if (arr._capacity < new_size)
            {
                ice::pod::array::set_capacity(arr, new_size);
            }
            arr._size = new_size;
        }

        template<typename T>
        inline void shrink(ice::pod::Array<T>& arr) noexcept
        {
            ice::pod::array::set_capacity(arr, arr._size);
        }

        template<typename T>
        inline void clear(ice::pod::Array<T>& arr) noexcept
        {
            ice::pod::array::resize(arr, 0);
        }

        template<typename T>
        inline auto span(ice::pod::Array<T>& arr, ice::u32 offset, ice::u32 count) noexcept -> ice::Span<T>
        {
            return ice::Span<T>{ arr }.subspan(offset, (count == ~0) ? std::dynamic_extent : size_t{ count });
        }

        template<typename T, typename U>
        inline void push_back(ice::pod::Array<T>& arr, U const& item) noexcept
        {
            if (arr._size == arr._capacity)
            {
                ice::pod::array::grow(arr);
            }

            arr._data[arr._size] = T{ item };
            arr._size += 1;
        }

        template<typename T>
        inline void push_back(ice::pod::Array<T>& arr, ice::pod::Array<T> const& items) noexcept
        {
            ice::pod::array::push_back(arr, ice::Span<T const>{ items });
        }

        template<typename T>
        inline void push_back(ice::pod::Array<T>& arr, ice::Span<T const> items) noexcept
        {
            uint32_t const required_capacity = arr._size + items.size(); // #todo free-function APi
            if (required_capacity > arr._capacity)
            {
                ice::pod::array::grow(arr, required_capacity);
            }

            ice::memcpy(ice::pod::array::end(arr), items.data(), items.size_bytes()); // #todo free-function API
            arr._size += items.size(); // #todo free-function API
        }

        template<typename T>
        inline void pop_back(ice::pod::Array<T>& arr, uint32_t count) noexcept
        {
            if (arr._size > count)
            {
                arr._size -= count;
            }
            else
            {
                arr._size = 0;
            }
        }

        template<typename T>
        inline auto begin(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::Iterator
        {
            return arr._data;
        }

        template<typename T>
        inline auto end(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::Iterator
        {
            return arr._data + arr._size;
        }

        template<typename T>
        inline auto rbegin(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::ReverseIterator
        {
            return typename ice::pod::Array<T>::ReverseIterator{ arr._data + arr._size };
        }

        template<typename T>
        inline auto rend(ice::pod::Array<T>& arr) noexcept -> typename ice::pod::Array<T>::ReverseIterator
        {
            return typename ice::pod::Array<T>::ReverseIterator{ arr._data };
        }


        template<typename T>
        inline auto size(ice::pod::Array<T> const& arr) noexcept -> uint32_t
        {
            return arr._size;
        }

        template<typename T>
        inline auto capacity(ice::pod::Array<T> const& arr) noexcept -> uint32_t
        {
            return arr._capacity;
        }

        template<typename T>
        inline bool any(ice::pod::Array<T> const& arr) noexcept
        {
            return arr._size != 0;
        }

        template<typename T>
        inline bool empty(ice::pod::Array<T> const& arr) noexcept
        {
            return arr._size == 0;
        }

        template<typename T>
        inline auto span(ice::pod::Array<T> const& arr, ice::u32 offset, ice::u32 count) noexcept -> ice::Span<T const>
        {
            return ice::Span<T const>{ arr }.subspan(offset, (count == ~0) ? std::dynamic_extent : size_t{ count });
        }

        template<typename T>
        inline auto begin(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstIterator
        {
            return arr._data;
        }

        template<typename T>
        inline auto end(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstIterator
        {
            return arr._data + arr._size;
        }

        template<typename T>
        inline auto rbegin(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstReverseIterator
        {
            return typename ice::pod::Array<T>::ConstReverseIterator{ arr._data + arr._size };
        }

        template<typename T>
        inline auto rend(ice::pod::Array<T> const& arr) noexcept -> typename ice::pod::Array<T>::ConstReverseIterator
        {
            return typename ice::pod::Array<T>::ConstReverseIterator{ arr._data };
        }

        template<typename T>
        inline auto front(ice::pod::Array<T> const& arr) noexcept -> T const&
        {
            // #todo assert
            return arr._data[0];
        }

        template<typename T>
        inline auto back(ice::pod::Array<T> const& arr) noexcept -> T const&
        {
            // #todo assert
            return arr._data[arr._size - 1];
        }

    } // namespace array

    using array::begin;
    using array::end;

} // namespace ice::pod::array
