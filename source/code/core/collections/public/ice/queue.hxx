/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/container_concepts.hxx>
#include <ice/container/basic_container.hxx>
#include <ice/container/resizable_container.hxx>
#include <ice/mem_initializers.hxx>

namespace ice
{

    //! \brief A double ended queue, build on a circular buffer.
    //!
    //! \details Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value is set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct Queue
        : public ice::container::BasicContainer
        , public ice::container::ResizableContainer
    {
        static_assert(
            Logic == ContainerLogic::Complex || ice::TrivialContainerLogicAllowed<Type>,
            "Collection element type is not allowed with 'Trivial' logic!"
        );

        using ValueType = Type;
        using ConstContainerValueType = Type const;
        using Iterator = Type*;
        using ReverseIterator = std::reverse_iterator<Type*>;
        using ConstIterator = Type const*;
        using ConstReverseIterator = std::reverse_iterator<Type const*>;
        using SizeType = ice::ncount;
        using ContainerTag = ice::concepts::ContiguousContainerTag;

        ice::Allocator* _allocator;
        ice::u32 _capacity;
        ice::u32 _count;
        ice::u32 _offset;
        Type* _data;

        inline explicit Queue(ice::Allocator& alloc) noexcept;
        inline Queue(Queue&& other) noexcept;
        inline Queue(Queue const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Queue() noexcept;

        // API Requirements Of: Container
        constexpr auto size() const noexcept -> ice::ncount { return { _count, sizeof(ValueType) }; }

        // API Requirements Of: Resizable Container
        constexpr auto capacity() const noexcept -> ice::ncount { return { _capacity, sizeof(ValueType) }; }
        constexpr void set_capacity(ice::ncount new_capacity) noexcept;
        constexpr void resize(ice::ncount new_size) noexcept;
        constexpr void clear() noexcept;

        // API Manipulation
        template<typename ItemType = Type>
            requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
        constexpr void push_front(ItemType&& item) noexcept;

        template<typename ItemType = Type>
            requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
        constexpr void push_back(ItemType&& item) noexcept;

        // TODO: Think of changing it to iterable container?
        template<ice::concepts::ContiguousContainer ContainerT>
            requires (ice::concepts::CompatibleContainer<Type, ContainerT>)
        constexpr void push_front(ContainerT const& other) noexcept;

        // TODO: Think of changing it to iterable container?
        template<ice::concepts::ContiguousContainer ContainerT>
            requires (ice::concepts::CompatibleContainer<Type, ContainerT>)
        constexpr void push_back(ContainerT const& other) noexcept;

        constexpr void pop_front(ice::ncount count = 1_count) noexcept;
        constexpr void pop_back(ice::ncount count = 1_count) noexcept;

        // Explicit functionality
        template<typename Self>
        constexpr auto front(this Self&& self) noexcept -> ice::container::ValueRef<Self>;
        template<typename Self>
        constexpr auto back(this Self&& self) noexcept -> ice::container::ValueRef<Self>;

        template<typename Self, typename Fn>
        constexpr void for_each(this Self&& self, Fn&& fn) noexcept;
        template<typename Self, typename Fn>
        constexpr void for_each_reverse(this Self&& self, Fn&& fn) noexcept;

        template<typename Self>
        constexpr auto take_front(this Self&& self, ice::Span<Type> out_values) noexcept -> ice::ncount;

        // API Requirements Of: Memory
        constexpr auto memory_view(this Queue& self) noexcept -> ice::Memory;

        // Operators and implicit conversions
        inline auto operator=(Queue&& other) noexcept -> Queue&;
        inline auto operator=(Queue const& other) noexcept -> Queue&
            requires std::copy_constructible<Type>;

        template<typename Self>
        constexpr auto operator[](
            this Self&& self, ice::nindex idx
        ) noexcept -> ice::container::ValueRef<Self>;
    };

    namespace queue::detail
    {

        template<typename Type>
        void destroy_head_items(ice::Queue<Type, ContainerLogic::Complex>& queue, ice::ncount destroy_count) noexcept
        {
            ice::u32 const raw_end_idx = queue._offset + destroy_count.u32();
            ice::nindex const start_idx = queue._offset;
            ice::nindex const end_idx = raw_end_idx % queue._capacity;

            // We got a wrapped case
            if (start_idx > end_idx)
            {
                ice::mem_destruct_n_at(queue._data + start_idx, queue._capacity - start_idx);
                ice::mem_destruct_n_at(queue._data, end_idx);
            }
            else
            {
                ice::mem_destruct_n_at(queue._data + start_idx, destroy_count);
            }
        }

        template<typename Type>
        void destroy_tail_items(ice::Queue<Type, ContainerLogic::Complex>& queue, ice::ncount destroy_count) noexcept
        {
            ice::u32 const raw_end_idx = queue._offset + queue._count;
            ice::nindex const start_idx = (raw_end_idx - destroy_count.u32()) % queue._capacity;
            ice::nindex const end_idx = raw_end_idx % queue._capacity;

            // We got a wrapped case
            if (start_idx > end_idx)
            {
                ice::mem_destruct_n_at(queue._data + start_idx, queue._capacity - start_idx);
                ice::mem_destruct_n_at(queue._data, end_idx);
            }
            else
            {
                ice::mem_destruct_n_at(queue._data + start_idx, destroy_count);
            }
        }

        template<typename Type>
        inline void copy_items_to_new_location(ice::Memory dest, ice::Queue<Type, ContainerLogic::Complex> const& queue) noexcept
        {
            ice::u32 const start_idx = queue._offset;
            ice::u32 const head_count = queue._capacity - start_idx;
            ice::usize const head_size = ice::size_of<Type> * head_count;

            ice::mem_copy_construct_n_at(dest, queue._data + start_idx, head_count);
            // Move destination pointer
            dest.size.value = static_cast<ice::usize::base_type>((dest.size - head_size).value);
            dest.location = ice::ptr_add(dest.location, ice::size_of<Type> * head_count);
            ice::mem_copy_construct_n_at(dest, queue._data, queue._count - head_count);
        }

        template<typename Type>
        inline void move_items_to_new_location(ice::Memory dest, ice::Queue<Type, ContainerLogic::Complex>& queue) noexcept
        {
            ice::u32 const start_idx = queue._offset;
            ice::u32 const head_count = queue._capacity - start_idx;
            ice::usize const head_size = ice::size_of<Type> * head_count;

            ice::mem_move_construct_n_at(dest, queue._data + start_idx, head_count);
            // Move destination pointer
            dest.size.value = static_cast<ice::usize::base_type>((dest.size - head_size).value);
            dest.location = ice::ptr_add(dest.location, ice::size_of<Type> *head_count);
            ice::mem_move_construct_n_at(dest, queue._data, queue._count - head_count);

            // Destroy the items left in the old queue memory.
            ice::mem_destruct_n_at(queue._data + start_idx, head_count);
            ice::mem_destruct_n_at(queue._data, queue._count - head_count);
        }

        template<typename Type>
        inline void copy_memory_to_new_location(ice::Memory dest, ice::Queue<Type, ContainerLogic::Trivial> const& queue) noexcept
        {
            ice::usize const total_size = ice::size_of<Type> * queue._count;

            ice::u32 const head_count = std::min(queue._offset + queue._count, queue._capacity) - queue._offset;
            ice::u32 const tail_count = queue._count - head_count;

            ice::usize const head_size = ice::size_of<Type> * head_count;
            ice::usize const head_end_offset = ice::size_of<Type> * head_count;
            ice::usize const tail_end_offset = ice::size_of<Type> * tail_count;

            ice::memcpy(
                dest,
                Data{
                    .location = queue._data + queue._offset,
                    .size = head_end_offset,
                    .alignment = ice::align_of<Type>
                }
            );
            // Move destination pointer
            dest.size.value = static_cast<ice::usize::base_type>((dest.size - head_size).value);
            dest.location = ice::ptr_add(dest.location, head_size);
            ice::memcpy(
                dest,
                Data{
                    .location = queue._data,
                    .size = tail_end_offset,
                    .alignment = ice::align_of<Type>
                }
            );
        }

    } // namespace queue::detail

    template<typename Type, ice::ContainerLogic Logic>
    inline Queue<Type, Logic>::Queue(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Queue<Type, Logic>::Queue(Queue&& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _count{ ice::exchange(other._count, 0) }
        , _offset{ ice::exchange(other._offset, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Queue<Type, Logic>::Queue(Queue const& other) noexcept requires std::copy_constructible<Type>
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _count{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        if (other._count > 0)
        {
            this->set_capacity(other.size());

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::queue::detail::copy_items_to_new_location(this->memory_view(), other);
            }
            else
            {
                ice::queue::detail::copy_memory_to_new_location(this->memory_view(), other);
            }

            _count = other._count;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline Queue<Type, Logic>::~Queue() noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::u32 const last_raw_idx = _offset + _count;
            if (last_raw_idx > _capacity)
            {
                ice::u32 const wrapped_count = last_raw_idx - _capacity;
                // Destroyes elements at the end of the ring buffer [_offset, count_until_capacity)
                ice::mem_destruct_n_at(_data + _offset, _count - wrapped_count);
                // Destroys wrapped tail elements [0, tail_size)
                ice::mem_destruct_n_at(_data, wrapped_count);
            }
            else
            {
                ice::mem_destruct_n_at(_data + _offset, _count);
            }
        }

        _allocator->deallocate(this->memory_view());
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Queue<Type, Logic>::operator=(Queue&& other) noexcept -> Queue&
    {
        if (this != &other)
        {
            this->set_capacity(0_count);

            _capacity = ice::exchange(other._capacity, 0);
            _count = ice::exchange(other._count, 0);
            _offset = ice::exchange(other._offset, 0);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Queue<Type, Logic>::operator=(Queue const& other) noexcept -> Queue&
        requires std::copy_constructible<Type>
    {
        if (this != &other)
        {
            this->clear();
            this->reserve(other.size());

            if (other._count > 0)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::queue::detail::copy_items_to_new_location(this->memory_view(), other);
                }
                else
                {
                    ice::queue::detail::copy_memory_to_new_location(this->memory_view(), other);
                }
            }

            _count = other._count;
        }
        return *this;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Queue<Type, Logic>::set_capacity(ice::ncount new_capacity) noexcept
    {
        if (new_capacity == _capacity)
        {
            return;
        }

        if (new_capacity < _count)
        {
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::queue::detail::destroy_tail_items(*this, _count - new_capacity);
            }

            _count = new_capacity.u32();
        }

        Type* new_data = nullptr;
        if (new_capacity > 0)
        {
            ice::AllocResult new_buffer = _allocator->allocate(ice::meminfo_of<Type> * new_capacity);
            if (_count > 0)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::queue::detail::move_items_to_new_location(new_buffer, *this);
                }
                else
                {
                    ice::queue::detail::copy_memory_to_new_location(new_buffer, *this);
                }
            }
            new_data = reinterpret_cast<Type*>(new_buffer.memory);
        }

        _allocator->deallocate(this->memory_view());
        _data = new_data;
        _capacity = new_capacity.u32();
        _offset = 0;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Queue<Type, Logic>::resize(ice::ncount new_size) noexcept
    {
        if (_capacity < new_size)
        {
            set_capacity(new_size);
        }

        // Even for trivial logic we construct items so at least the default ctor is called.
        if (new_size > _count)
        {
            ice::ncount const missing_items = new_size - _count;
            ice::nindex const start_idx = (_offset + _count) % _capacity;

            ice::nindex const end_idx = ice::min<ice::nindex>(start_idx + missing_items, _capacity);
            ice::nindex const wrapped_end_idx = missing_items - (end_idx - start_idx);

            // Construct until we hit end of the queue buffer
            ice::mem_default_construct_n_at<Type>(
                Memory{
                    .location = _data + start_idx,
                    .size = ice::size_of<Type> * (end_idx - start_idx),
                    .alignment = ice::align_of<Type>
                },
                (end_idx - start_idx)
            );
            // Construct the rest wrapped around the buffer
            ice::mem_default_construct_n_at<Type>(
                Memory{
                    .location = _data,
                    .size = ice::size_of<Type> * wrapped_end_idx,
                    .alignment = ice::align_of<Type>
                },
                wrapped_end_idx
            );
        }
        else if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::queue::detail::destroy_tail_items(*this, _count - new_size);
        }

        _count = new_size.u32();
    }

    template<typename Type, ice::ContainerLogic Logic>
    constexpr void ice::Queue<Type, Logic>::clear() noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::queue::detail::destroy_head_items(*this, size());
        }

        _count = 0;
        _offset = 0;
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename ItemType> requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
    inline constexpr void ice::Queue<Type, Logic>::push_front(ItemType&& item) noexcept
    {
        if (_count == _capacity)
        {
            this->grow();
        }

        if (_offset == 0)
        {
            _offset = _capacity;
        }

        _offset -= 1;

        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_construct_at<Type>(
                ice::ptr_add(this->memory_view(), ice::size_of<Type> * _offset),
                ice::forward<Type>(item)
            );
        }
        else
        {
            _data[_offset] = Type{ item };
        }

        _count += 1;
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename ItemType> requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
    inline constexpr void ice::Queue<Type, Logic>::push_back(ItemType&& item) noexcept
    {
        if (_count == _capacity)
        {
            this->grow();
        }

        ice::u32 const item_idx = (_offset + _count) % _capacity;
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_construct_at<Type>(
                ice::ptr_add(this->memory_view(), ice::size_of<Type> * item_idx),
                ice::forward<Type>(item)
            );
        }
        else
        {
            _data[item_idx] = Type{ item };
        }

        _count += 1;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Queue<Type, Logic>::pop_front(ice::ncount count) noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::queue::detail::destroy_head_items(*this, ice::min(count, this->size()));
        }

        if (_count > count)
        {
            _count -= count.u32();
            _offset = (_offset + count.u32()) % _capacity;
        }
        else
        {
            _count = 0;
            _offset = 0;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Queue<Type, Logic>::pop_back(ice::ncount count) noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::queue::detail::destroy_tail_items(*this, ice::min(count, this->size()));
        }

        if (_count > count)
        {
            _count -= count.u32();
        }
        else
        {
            _count = 0;
            _offset = 0;
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<ice::concepts::ContiguousContainer ContainerT>
        requires (ice::concepts::CompatibleContainer<Type, ContainerT>)
    inline constexpr void ice::Queue<Type, Logic>::push_back(ContainerT const& other) noexcept
    {
        ice::ncount const other_count = other.size();
        ice::ncount const required_capacity = _count + other_count;
        if (required_capacity > _capacity)
        {
            this->grow(required_capacity);
        }

        ice::nindex const start_idx = (_offset + _count) % _capacity;
        ice::nindex const end_idx = ice::min<nindex>(_capacity, start_idx + other_count);

        // The space can we use before wrapping around the buffer.
        ice::ncount const head_space = end_idx - start_idx;
        // The space after the wrapped buffer we still need to allocate. Can be 0.
        ice::ncount const tail_space = other_count - head_space;

        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_copy_construct_n_at<Type>(
                Memory{
                    .location = _data + start_idx,
                    .size = ice::size_of<Type> * head_space,
                    .alignment = ice::align_of<Type>
                },
                other.data(),
                head_space
            );
            ice::mem_copy_construct_n_at<Type>(
                Memory{
                    .location = _data,
                    .size = ice::size_of<Type> * tail_space,
                    .alignment = ice::align_of<Type>
                },
                other.data() + head_space,
                tail_space
            );
        }
        else
        {
            ice::memcpy(
                Memory{
                    .location = _data + start_idx,
                    .size = ice::size_of<Type> * head_space,
                    .alignment = ice::align_of<Type>
                },
                Data{
                    .location = other.data(),
                    .size = ice::size_of<Type> * head_space,
                    .alignment = ice::align_of<Type>
                }
            );
            ice::memcpy(
                Memory{
                    .location = _data,
                    .size = ice::size_of<Type> * tail_space,
                    .alignment = ice::align_of<Type>
                },
                Data{
                    .location = other.data() + head_space,
                    .size = ice::size_of<Type> * tail_space,
                    .alignment = ice::align_of<Type>
                }
            );
        }

        _count += other.size().u32();
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto Queue<Type, Logic>::front(this Self&& self) noexcept -> ice::container::ValueRef<Self>
    {
        return self._data[self._offset];
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto Queue<Type, Logic>::back(this Self&& self) noexcept -> ice::container::ValueRef<Self>
    {
        return self._data[((self._offset + self._count) - 1) % self._capacity];
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self, typename Fn>
    inline constexpr void ice::Queue<Type, Logic>::for_each(this Self&& self, Fn&& fn) noexcept
    {
        if (self._count == 0)
        {
            return;
        }

        ice::u32 const first_part = ice::min(self._offset + self._count, self._capacity);
        ice::u32 const second_part = (self._offset + self._count) - first_part;

        for (ice::u32 idx = self._offset; idx < first_part; ++idx)
        {
            ice::forward<Fn>(fn)(self._data[idx]);
        }

        for (ice::u32 idx = 0; idx < second_part; ++idx)
        {
            ice::forward<Fn>(fn)(self._data[idx]);
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self, typename Fn>
    inline constexpr void ice::Queue<Type, Logic>::for_each_reverse(this Self&& self, Fn&& fn) noexcept
    {
        if (self._count == 0)
        {
            return;
        }

        ice::u32 const first_part = ice::min(self._offset + self._count, self._capacity);
        ice::u32 const second_part = (self._offset + self._count) - first_part;

        if (second_part > 0)
        {
            for (ice::u32 idx = second_part - 1; idx > 0; --idx)
            {
                ice::forward<Fn>(fn)(self._data[idx]);
            }

            ice::forward<Fn>(fn)(self._data[0]);
        }

        for (ice::u32 idx = first_part - 1; idx > self._offset; --idx)
        {
            ice::forward<Fn>(fn)(self._data[idx]);
        }

        ice::forward<Fn>(fn)(self._data[self._offset]);
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    inline constexpr auto ice::Queue<Type, Logic>::take_front(
        this Self&& self,
        ice::Span<Type> out_values
    ) noexcept -> ice::ncount
    {
        ice::ncount const taken_items = ice::min(out_values.size(), self.size());

        // (offset, end][0, remaining)
        ice::ncount const first_part = ice::min<ice::ncount>(self._offset + taken_items, self._capacity);
        ice::ncount const second_part = (self._offset + taken_items) - first_part;
        ice::ncount const first_part_count = first_part - self._offset;

        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_move_n_to(out_values.begin(), self._data + self._offset, first_part_count);
            ice::mem_move_n_to(out_values.begin() + first_part_count, self._data, second_part);
        }
        else
        {
            ice::memcpy(out_values.begin(), self._data + self._offset, ice::size_of<Type> * first_part_count);
            ice::memcpy(out_values.begin() + first_part_count, self._data, ice::size_of<Type> * second_part);
        }

        self.pop_front(taken_items);
        return taken_items;
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto Queue<Type, Logic>::memory_view(this Queue& self) noexcept -> ice::Memory
    {
        return ice::Memory{
            .location = self._data,
            .size = self.capacity(),
            .alignment = ice::align_of<ValueType>
        };
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename Self>
    constexpr auto Queue<Type, Logic>::operator[](
        this Self&& self, ice::nindex idx
    ) noexcept -> ice::container::ValueRef<Self>
    {
        return self._data[(idx + self._offset) % self._capacity];
    }


} // namespace ice
