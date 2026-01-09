/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::Array(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _data{ nullptr }
    { }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::Array(Array&& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _count{ ice::exchange(other._count, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::Array(Array const& other) noexcept
        requires std::copy_constructible<Type>
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _count{ 0 }
        , _data{ nullptr }
    {
        if (other._count > 0)
        {
            ice::array::set_capacity(*this, other._count);

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    ice::array::memory(*this),
                    other._data,
                    other._count
                );
            }
            else
            {
                ice::memcpy(
                    ice::array::memory(*this),
                    ice::array::data_view(other)
                );
            }

            _count = other._count;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::~Array() noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_destruct_n_at(_data, _count);
        }

        _allocator->deallocate(ice::array::memory(*this));
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::Array(
        ice::Allocator& alloc,
        ice::Span<Type const> values
    ) noexcept
        requires std::copy_constructible<Type>
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _data{ nullptr }
    {
        if (values.not_empty())
        {
            ice::array::set_capacity(*this, values.size().u32());

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    ice::array::memory(*this),
                    values.data(),
                    values.size()
                );
            }
            else
            {
                ice::memcpy(_data, values.data(), values.size().bytes());
            }

            _count = values.size().u32();
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Array<Type, Logic>::operator=(Array&& other) noexcept -> Array&
    {
        if (this != &other)
        {
            ice::array::set_capacity(*this, 0);

            _allocator = other._allocator;
            _capacity = ice::exchange(other._capacity, 0);
            _data = ice::exchange(other._data, nullptr);
            _count = ice::exchange(other._count, 0);
        }
        return *this;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Array<Type, Logic>::operator=(Array const& other) noexcept -> Array&
        requires std::copy_constructible<Type>
    {
        if (this != &other)
        {
            ice::array::clear(*this);
            ice::array::reserve(*this, other._capacity);

            if (other._count > 0)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::mem_copy_construct_n_at(
                        ice::array::memory(*this),
                        other._data,
                        other._count
                    );
                }
                else
                {
                    ice::memcpy(
                        ice::array::memory(*this),
                        ice::array::data_view(other)
                    );
                }
            }

            _count = other._count;
        }
        return *this;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Array<Type, Logic>::operator[](ice::u32 idx) noexcept -> Type&
    {
        // TODO: Assert
        return _data[idx];
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Array<Type, Logic>::operator[](ice::u32 idx) const noexcept -> Type const&
    {
        // TODO: Assert
        return _data[idx];
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::operator ice::Span<Type>() noexcept
    {
        return Span{ _data, _count };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Array<Type, Logic>::operator ice::Span<Type const>() const noexcept
    {
        return Span{ _data, _count };
    }

    namespace array
    {

        template<typename Type, ice::ContainerLogic Logic>
        inline void set_capacity(ice::Array<Type, Logic>& arr, ice::u32 new_capacity) noexcept
        {
            if (new_capacity == arr._capacity)
            {
                return;
            }

            if (new_capacity < arr._count)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::mem_destruct_n_at(arr._data + new_capacity, arr._count - new_capacity);
                }

                arr._count = new_capacity;
            }

            Type* new_data = nullptr;
            if (new_capacity > 0)
            {
                ice::AllocResult new_buffer = arr._allocator->allocate(ice::meminfo_of<Type> * new_capacity);
                ICE_ASSERT_CORE(new_buffer.memory != nullptr);
                if (arr._count > 0)
                {
                    if constexpr (Logic == ContainerLogic::Complex)
                    {
                        ice::mem_move_construct_n_at(new_buffer, arr._data, arr._count);
                        ice::mem_destruct_n_at(arr._data, arr._count);
                    }
                    else
                    {
                        ice::memcpy(new_buffer, ice::array::data_view(arr));
                    }
                }
                new_data = reinterpret_cast<Type*>(new_buffer.memory);
            }

            arr._allocator->deallocate(ice::array::memory(arr));
            arr._data = new_data;
            arr._capacity = new_capacity;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void reserve(ice::Array<Type, Logic>& arr, ice::u32 min_capacity) noexcept
        {
            if (arr._capacity < min_capacity)
            {
                ice::array::set_capacity(arr, min_capacity);
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void grow(ice::Array<Type, Logic>& arr, ice::u32 min_capacity) noexcept
        {
            ice::u32 new_capacity = arr._capacity * 2 + 4;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            ice::array::set_capacity(arr, new_capacity);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void resize(ice::Array<Type, Logic>& arr, ice::u32 new_count) noexcept
        {
            if (arr._capacity < new_count)
            {
                ice::array::set_capacity(arr, new_count);
            }

            if (new_count > arr._count)
            {
                ice::u32 const missing_items = new_count - arr._count;

                // Even for trivial logic we construct items so at least the default ctor is called.
                ice::mem_construct_n_at<Type>(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type> * missing_items, .alignment = ice::align_of<Type> },
                    missing_items
                );
            }
            else if constexpr (Logic == ContainerLogic::Complex)
            {
                static_assert(Logic != ContainerLogic::Trivial);
                ice::u32 const destroyed_items = arr._count - new_count;

                ice::mem_destruct_n_at(
                    arr._data + new_count,
                    destroyed_items
                );
            }

            arr._count = new_count;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void shrink(ice::Array<Type, Logic>& arr) noexcept
        {
            ice::array::set_capacity(arr, arr._count);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void clear(ice::Array<Type, Logic>& arr) noexcept
        {
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_n_at(arr._data, arr._count);
            }

            arr._count = 0;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto slice(ice::Array<Type, Logic>& arr, ice::u32 from_idx, ice::u32 count) noexcept -> ice::Span<Type>
        {
            ice::Span<Type> arr_span{ arr };
            return arr_span.subspan(from_idx, count);
        }

        template<typename Type, ice::ContainerLogic Logic>
            requires std::move_constructible<Type>
        inline void push_back(ice::Array<Type, Logic>& arr, Type&& item) noexcept
        {
            if (arr._count == arr._capacity)
            {
                ice::array::grow(arr);
            }

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_move_construct_at<Type>(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    ice::forward<Type>(item)
                );
            }
            else
            {
                arr._data[arr._count] = Type{ item };
            }

            arr._count += 1;
        }

        template<typename Type, ice::ContainerLogic Logic, typename Value>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void push_back(ice::Array<Type, Logic>& arr, Value const& item) noexcept
        {
            if (arr._count == arr._capacity)
            {
                ice::array::grow(arr);
            }

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_at<Type>(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    item
                );
            }
            else
            {
                arr._data[arr._count] = Type{ item };
            }

            arr._count += 1;
        }

        template<typename Type, ice::ContainerLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Array<Type, Logic> const& items) noexcept
        {
            return ice::array::push_back(arr, ice::array::slice(items));
        }

        template<typename Type, ice::ContainerLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Type const> items) noexcept
        {
            ice::u32 const required_capacity = arr._count + items.size().u32();
            if (required_capacity > arr._capacity)
            {
                ice::array::grow(arr, required_capacity);
            }

            ice::u32 const missing_items = required_capacity - arr._count;
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at<Type>(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type> * missing_items, .alignment = ice::align_of<Type> },
                    items.data(),
                    items.size().u32()
                );
            }
            else
            {
                ice::memcpy(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type> * missing_items, .alignment = ice::align_of<Type> },
                    items.data_view()
                );
            }

            arr._count += items.size().u32();
        }

        template<typename Type, ice::ContainerLogic Logic, typename Source>
            requires std::copy_constructible<Type> && (std::is_same_v<Type, Source> == false)
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Source const> items, Type(*fn)(Source const&) noexcept) noexcept
        {
            ice::u32 const required_capacity = arr._count + items.size();
            if (required_capacity > arr._capacity)
            {
                ice::array::grow(arr, required_capacity);
            }

            ice::u32 const missing_items = required_capacity - arr._count;
            for (ice::u32 src_idx = 0; src_idx < missing_items; ++src_idx)
            {
                ice::array::push_back(arr, fn(items[src_idx]));
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void pop_back(ice::Array<Type, Logic>& arr, ice::u32 count /*= 1*/) noexcept
        {
            count = ice::min(count, arr._count);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_n_at(arr._data + (arr._count - count), count);
            }

            if (arr._count > count)
            {
                arr._count -= count;
            }
            else
            {
                arr._count = 0;
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto begin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator
        {
            return arr._data;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto end(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator
        {
            return arr._data + arr._count;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto rbegin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator
        {
            return typename ice::Array<Type, Logic>::ReverseIterator{ arr._data + arr._count };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto rend(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator
        {
            return typename ice::Array<Type, Logic>::ReverseIterator{ arr._data };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto front(ice::Array<Type, Logic>& arr) noexcept -> Type&
        {
            // #todo assert
            return arr._data[0];
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto back(ice::Array<Type, Logic>& arr) noexcept -> Type&
        {
            // #todo assert
            return arr._data[arr._count - 1];
        }



        template<typename Type, ice::ContainerLogic Logic>
        inline auto count(ice::Array<Type, Logic> const& arr) noexcept -> ice::u32
        {
            return arr._count;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto capacity(ice::Array<Type, Logic> const& arr) noexcept -> ice::u32
        {
            return arr._capacity;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto size_bytes(ice::Array<Type, Logic> const& arr) noexcept -> ice::usize
        {
            return ice::size_of<Type> * arr._count;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline bool any(ice::Array<Type, Logic> const& arr) noexcept
        {
            return arr._count != 0;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline bool empty(ice::Array<Type, Logic> const& arr) noexcept
        {
            return arr._count == 0;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto slice(ice::Array<Type, Logic> const& arr, ice::u32 from_idx, ice::u32 count) noexcept -> ice::Span<Type const>
        {
            ice::Span<Type const> arr_span{ arr };
            return arr_span.subspan(from_idx, count);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto slice(ice::Array<Type, Logic> const& arr, ice::ref32 ref) noexcept -> ice::Span<Type const>
        {
            ice::Span<Type const> arr_span{ arr };
            return arr_span.subspan(ref.offset, ref.size);
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto begin(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstIterator
        {
            return arr._data;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto end(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstIterator
        {
            return arr._data + arr._count;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto rbegin(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstReverseIterator
        {
            return typename ice::Array<Type, Logic>::ConstReverseIterator{ arr._data + arr._count };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto rend(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstReverseIterator
        {
            return typename ice::Array<Type, Logic>::ConstReverseIterator{ arr._data };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto front(ice::Array<Type, Logic> const& arr) noexcept -> Type const&
        {
            // #todo assert
            return arr._data[0];
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto back(ice::Array<Type, Logic> const& arr) noexcept -> Type const&
        {
            // #todo assert
            return arr._data[arr._size - 1];
        }



        template<typename Type, ice::ContainerLogic Logic>
        inline auto data_view(ice::Array<Type, Logic> const& arr) noexcept -> ice::Data
        {
            return ice::Data{
                .location = arr._data,
                .size = ice::size_of<Type> * arr._count,
                .alignment = ice::align_of<Type>
            };
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto memory(ice::Array<Type, Logic>& arr) noexcept -> ice::Memory
        {
            return ice::Memory{
                .location = arr._data,
                .size = ice::size_of<Type> * arr._capacity,
                .alignment = ice::align_of<Type>
            };
        }

        template<typename Type>
        inline auto memset(ice::Array<Type, ice::ContainerLogic::Trivial>& arr, ice::u8 value) noexcept -> ice::Memory
        {
            ice::Memory const mem = ice::array::memory(arr);
            ice::memset(mem.location, value, mem.size.value);
            return mem;
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline auto meminfo(ice::Array<Type, Logic> const& arr) noexcept -> ice::meminfo
        {
            return ice::meminfo_of<Type> * ice::array::count(arr);
        }

    } // namespace array

} // namespace ice
