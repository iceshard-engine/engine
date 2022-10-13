namespace ice
{

    namespace queue::detail
    {

        template<typename Type>
        inline void destroy_head_items(ice::Queue<Type, CollectionLogic::Complex>& queue, ice::ucount destroy_count) noexcept;

        template<typename Type>
        inline void destroy_tail_items(ice::Queue<Type, CollectionLogic::Complex>& queue, ice::ucount destroy_count) noexcept;

        template<typename Type>
        inline void copy_items_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::Complex> const& queue) noexcept;

        template<typename Type>
        inline void move_items_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::Complex> const& queue) noexcept;

        template<typename Type>
        inline void copy_memory_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::PlainOldData> const& queue) noexcept;


    } // namespace queue::detail

    template<typename Type, ice::CollectionLogic Logic>
    inline Queue<Type, Logic>::Queue(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
        , _capacity{ 0 }
        , _count{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline Queue<Type, Logic>::Queue(Queue&& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _count{ ice::exchange(other._count, 0) }
        , _offset{ ice::exchange(other._offset, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline Queue<Type, Logic>::Queue(Queue const& other) noexcept
        requires std::copy_constructible<Type>
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _count{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        if (other._count > 0)
        {
            ice::queue::set_capacity(*this, other._count);

            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::queue::detail::copy_items_to_new_location(ice::queue::memory(*this), other);
            }
            else
            {
                ice::queue::detail::copy_memory_to_new_location(ice::queue::memory(*this), other);
            }

            _count = other._count;
        }
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline Queue<Type, Logic>::~Queue() noexcept
    {
        if constexpr (Logic == CollectionLogic::Complex)
        {
            ice::ucount const last_raw_idx = _offset + _count;
            if (last_raw_idx > _capacity)
            {
                ice::ucount const wrapped_count = last_raw_idx - _capacity;
                ice::mem_destruct_n_at(_data + _offset, _count - wrapped_count); // Destroyes elements at the end of the ring buffer [_offset, count_until_capacity)
                ice::mem_destruct_n_at(_data, wrapped_count); // Destroys wrapped tail elements [0, tail_size)
            }
            else
            {
                ice::mem_destruct_n_at(_data + _offset, _count);
            }
        }

        _allocator->deallocate(ice::queue::memory(*this));
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline auto Queue<Type, Logic>::operator=(Queue&& other) noexcept -> Queue&
    {
        if (this != &other)
        {
            ice::queue::set_capacity(*this, 0);

            _capacity = ice::exchange(other._capacity, 0);
            _count = ice::exchange(other._count, 0);
            _offset = ice::exchange(other._offset, 0);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline auto Queue<Type, Logic>::operator=(Queue const& other) noexcept -> Queue&
        requires std::copy_constructible<Type>
    {
        if (this != &other)
        {
            ice::queue::clear(*this);
            ice::queue::reserve(*this, other._capacity);

            _data = other._data;
            _count = other._count;
            _offset = other._offset;
        }
        return *this;
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline auto Queue<Type, Logic>::operator[](ice::ucount idx) noexcept -> Type&
    {
        return _data[(idx + _offset) % _capacity];
    }

    template<typename Type, ice::CollectionLogic Logic>
    inline auto Queue<Type, Logic>::operator[](ice::ucount idx) const noexcept -> Type const&
    {
        return _data[(idx + _offset) % _capacity];
    }

    namespace queue::detail
    {

        template<typename Type>
        void destroy_head_items(ice::Queue<Type, CollectionLogic::Complex>& queue, ice::ucount destroy_count) noexcept
        {
            ice::ucount const raw_end_idx = queue._offset + destroy_count;
            ice::ucount const start_idx = queue._offset;
            ice::ucount const end_idx = raw_end_idx % queue._capacity;

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
        void destroy_tail_items(ice::Queue<Type, CollectionLogic::Complex>& queue, ice::ucount destroy_count) noexcept
        {
            ice::ucount const raw_end_idx = queue._offset + queue._count;
            ice::ucount const start_idx = (raw_end_idx - destroy_count) % queue._capacity;
            ice::ucount const end_idx = raw_end_idx % queue._capacity;

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
        inline void copy_items_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::Complex> const& queue) noexcept
        {
            ice::ucount const start_idx = queue._offset;
            ice::ucount const head_count = queue._capacity - start_idx;
            ice::usize const head_size = ice::size_of<Type> * head_count;

            ice::mem_copy_construct_n_at(dest, queue._data + start_idx, head_count);
            // Move destination pointer
            dest.size.value = static_cast<ice::usize::base_type>((dest.size - head_size).value);
            dest.location = ice::ptr_add(dest.location, ice::size_of<Type> * head_count);
            ice::mem_copy_construct_n_at(dest, queue._data, queue._count - head_count);
        }

        template<typename Type>
        inline void move_items_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::Complex> const& queue) noexcept
        {
            ice::ucount const start_idx = queue._offset;
            ice::ucount const head_count = queue._capacity - start_idx;
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
        inline void copy_memory_to_new_location(ice::Memory dest, ice::Queue<Type, CollectionLogic::PlainOldData> const& queue) noexcept
        {
            ice::usize const total_size = ice::size_of<Type> * queue._count;

            ice::ucount const head_count = std::min(queue._offset + queue._count, queue._capacity) - queue._offset;
            ice::ucount const tail_count = queue._count - head_count;

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

    namespace queue
    {

        template<typename Type, ice::CollectionLogic Logic>
        inline void set_capacity(ice::Queue<Type, Logic>& queue, ice::ucount new_capacity) noexcept
        {
            if (new_capacity == queue._capacity)
            {
                return;
            }

            if (new_capacity < queue._count)
            {
                if constexpr (Logic == CollectionLogic::Complex)
                {
                    ice::queue::detail::destroy_tail_items(queue, queue._count - new_capacity);
                }

                queue._count = new_capacity;
            }

            Type* new_data = nullptr;
            if (new_capacity > 0)
            {
                ice::AllocResult new_buffer = queue._allocator->allocate(ice::meminfo_of<Type> *new_capacity);
                if (queue._count > 0)
                {
                    if constexpr (Logic == CollectionLogic::Complex)
                    {
                        ice::queue::detail::move_items_to_new_location(new_buffer, queue);
                    }
                    else
                    {
                        ice::queue::detail::copy_memory_to_new_location(new_buffer, queue);
                    }
                }
                new_data = reinterpret_cast<Type*>(new_buffer.result);
            }

            queue._allocator->deallocate(ice::queue::memory(queue));
            queue._data = new_data;
            queue._capacity = new_capacity;
            queue._offset = 0; // Ofset is set to 0, as moving memory between buffers re-arranges the items.
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void reserve(ice::Queue<Type, Logic>& queue, ice::ucount min_capacity) noexcept
        {
            if (queue._capacity < min_capacity)
            {
                ice::queue::set_capacity(queue, min_capacity);
            }
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void grow(ice::Queue<Type, Logic>& queue, ice::ucount min_capacity /*= 0*/) noexcept
        {
            ice::ucount new_capacity = queue._count * 2 + 4;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            ice::queue::set_capacity(queue, new_capacity);
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void resize(ice::Queue<Type, Logic>& queue, ice::ucount new_count) noexcept
        {
            if (queue._capacity < new_count)
            {
                ice::queue::set_capacity(queue, new_count);
            }

            if constexpr (Logic == CollectionLogic::Complex)
            {
                if (new_count > queue._count)
                {
                    ice::ucount const missing_items = new_count - queue._count;
                    ice::ucount const start_idx = (queue._offset + queue._count) % queue._capacity;

                    ice::ucount const end_idx = ice::min(start_idx + missing_items, queue._capacity);
                    ice::ucount const wrapped_end_idx = missing_items - (end_idx - start_idx);

                    // Construct until we hit end of the queue buffer
                    ice::mem_construct_n_at<Type>(
                        Memory{
                            .location = queue._data + start_idx,
                            .size = ice::size_of<Type> * (end_idx - start_idx),
                            .alignment = ice::align_of<Type>
                        },
                        (end_idx - start_idx)
                    );
                    // Construct the rest wrapped around the buffer
                    ice::mem_construct_n_at<Type>(
                        Memory{
                            .location = queue._data,
                            .size = ice::size_of<Type> * wrapped_end_idx,
                            .alignment = ice::align_of<Type>
                        },
                        wrapped_end_idx
                    );
                }
                else
                {
                    ice::queue::detail::destroy_tail_items(queue, queue._count - new_count);
                }
            }

            queue._count = new_count;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void shrink(ice::Queue<Type, Logic>& queue) noexcept
        {
            ice::queue::set_capacity(queue, queue._count);
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void clear(ice::Queue<Type, Logic>& queue) noexcept
        {
            ice::queue::resize(queue, 0);
            queue._offset = 0;
        }

        template<typename Type, ice::CollectionLogic Logic>
            requires std::move_constructible<Type>
        inline void push_back(ice::Queue<Type, Logic>& queue, Type&& item) noexcept
        {
            if (queue._count == queue._capacity)
            {
                ice::queue::grow(queue);
            }

            ice::ucount const item_idx = (queue._offset + queue._count) % queue._capacity;
            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::mem_move_construct_at<Type>(
                    Memory{ .location = queue._data + item_idx, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    ice::forward<Type>(item)
                );
            }
            else
            {
                queue._data[item_idx] = Type{ item };
            }

            queue._count += 1;
        }

        template<typename Type, ice::CollectionLogic Logic, typename Value>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void push_back(ice::Queue<Type, Logic>& queue, Value const& item) noexcept
        {
            if (queue._count == queue._capacity)
            {
                ice::queue::grow(queue);
            }

            ice::ucount const item_idx = (queue._offset + queue._count) % queue._capacity;
            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::mem_copy_construct_at<Type>(
                    Memory{ .location = queue._data + item_idx, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    item
                );
            }
            else
            {
                queue._data[item_idx] = Type{ item };
            }

            queue._count += 1;
        }

        template<typename Type, ice::CollectionLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Queue<Type, Logic>& queue, ice::Queue<Type, Logic> const& items) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Queue<Type, Logic>& queue, ice::Span<Type const> items) noexcept
        {
            ice::ucount const required_capacity = queue._count + ice::span::count(items);
            if (required_capacity > queue._capacity)
            {
                ice::queue::grow(queue, required_capacity);
            }

            ice::ucount const span_count = ice::span::count(items);
            ice::ucount const start_idx = (queue._offset + queue._count) % queue._capacity;
            ice::ucount const end_idx = ice::min(queue._capacity, start_idx + span_count);

            // The space can we use before wrapping around the buffer.
            ice::ucount const head_space = end_idx - start_idx;
            // The space after the wrapped buffer we still need to allocate. Can be 0.
            ice::ucount const tail_space = span_count - head_space;

            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::mem_copy_construct_n_at<Type>(
                    Memory{
                        .location = queue._data + start_idx,
                        .size = ice::size_of<Type> * head_space,
                        .alignment = ice::align_of<Type>
                    },
                    ice::span::data(items),
                    head_space
                );
                ice::mem_copy_construct_n_at<Type>(
                    Memory{
                        .location = queue._data,
                        .size = ice::size_of<Type> * tail_space,
                        .alignment = ice::align_of<Type>
                    },
                    ice::span::data(items) + head_space,
                    tail_space
                );
            }
            else
            {
                ice::memcpy(
                    Memory{
                        .location = queue._data + start_idx,
                        .size = ice::size_of<Type> * head_space,
                        .alignment = ice::align_of<Type>
                    },
                    Data{
                        .location = ice::span::data(items),
                        .size = ice::size_of<Type> * head_space,
                        .alignment = ice::align_of<Type>
                    }
                );
                ice::memcpy(
                    Memory{
                        .location = queue._data,
                        .size = ice::size_of<Type> * tail_space,
                        .alignment = ice::align_of<Type>
                    },
                    Data{
                        .location = ice::span::data(items) + head_space,
                        .size = ice::size_of<Type> * tail_space,
                        .alignment = ice::align_of<Type>
                    }
                );
            }

            queue._count += ice::span::count(items);
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline void pop_back(ice::Queue<Type, Logic>& queue, ice::u32 count /*= 1*/) noexcept
        {
            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::queue::detail::destroy_tail_items(queue, ice::min(count, queue._count));
            }

            if (queue._count > count)
            {
                queue._count -= count;
            }
            else
            {
                queue._count = 0;
                queue._offset = 0;
            }
        }

        template<typename Type, ice::CollectionLogic Logic>
            requires std::move_constructible<Type>
        inline void push_front(ice::Queue<Type, Logic>& queue, Type&& item) noexcept
        {
            if (queue._count == queue._capacity)
            {
                ice::queue::grow(queue);
            }

            if (queue._offset == 0)
            {
                queue._offset = queue._capacity;
            }

            queue._offset -= 1;

            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::mem_move_construct_at<Type>(
                    Memory{ .location = queue._data + queue._offset, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    ice::forward<Type>(item)
                );
            }
            else
            {
                queue._data[queue._offset] = Type{ item };
            }

            queue._count += 1;
        }

        template<typename Type, ice::CollectionLogic Logic, typename Value>
            requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
        inline void push_front(ice::Queue<Type, Logic>& queue, Value const& item) noexcept
        {
            if (queue._count == queue._capacity)
            {
                ice::queue::grow(queue);
            }

            if (queue._offset == 0)
            {
                queue._offset = queue._capacity;
            }

            ice::ucount const item_idx = queue._offset - 1;
            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::mem_copy_construct_at<Type>(
                    Memory{ .location = queue._data + item_idx, .size = ice::size_of<Type>, .alignment = ice::align_of<Type> },
                    item
                );
            }
            else
            {
                queue._data[item_idx] = item;
            }

            queue._count += 1;
        }

        template<typename Type, ice::CollectionLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_front(ice::Queue<Type, Logic>& queue, ice::Queue<Type, Logic> const& items) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_front(ice::Queue<Type, Logic>& queue, ice::Span<Type const> items) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline void pop_front(ice::Queue<Type, Logic>& queue, ice::ucount count /*= 1*/) noexcept
        {
            if constexpr (Logic == CollectionLogic::Complex)
            {
                ice::queue::detail::destroy_head_items(queue, ice::min(count, queue._count));
            }

            if (queue._count > count)
            {
                queue._count -= count;
                queue._offset = (queue._offset + count) % queue._capacity;
            }
            else
            {
                queue._count = 0;
                queue._offset = 0;
            }
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto front(ice::Queue<Type, Logic>& queue) noexcept -> Type&
        {
            return queue._data[queue._offset];
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto back(ice::Queue<Type, Logic>& queue) noexcept -> Type&
        {
            return queue._data[((queue._offset + queue._count) - 1) % queue._capacity];
        }


        template<typename Type, ice::CollectionLogic Logic>
        inline auto count(ice::Queue<Type, Logic> const& queue) noexcept -> ice::ucount
        {
            return queue._count;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto capacity(ice::Queue<Type, Logic> const& queue) noexcept -> ice::ucount
        {
            return queue._capacity;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto size_bytes(ice::Queue<Type, Logic> const& queue) noexcept -> ice::usize
        {
            return ice::queue::count(queue) * ice::size_of<Type>;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline bool any(ice::Queue<Type, Logic> const& queue) noexcept
        {
            return queue._count != 0;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline bool empty(ice::Queue<Type, Logic> const& queue) noexcept
        {
            return queue._count == 0;
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto front(ice::Queue<Type, Logic> const& queue) noexcept -> Type const&
        {
            return queue._data[queue._offset];
        }

        template<typename Type, ice::CollectionLogic Logic>
        inline auto back(ice::Queue<Type, Logic> const& queue) noexcept -> Type const&
        {
            return queue._data[((queue._offset + queue._count) - 1) % queue._capacity];
        }

        template<typename Type, ice::CollectionLogic Logic, typename Fn>
        inline void for_each(ice::Queue<Type, Logic> const& queue, Fn&& fn) noexcept;

        template<typename Type, ice::CollectionLogic Logic>
        inline auto memory(ice::Queue<Type, Logic>& queue) noexcept -> ice::Memory
        {
            return ice::Memory{
                .location = queue._data,
                .size = ice::size_of<Type> * queue._capacity,
                .alignment = ice::align_of<Type>
            };
        }

    } // namespace queue

} // namespace ice

