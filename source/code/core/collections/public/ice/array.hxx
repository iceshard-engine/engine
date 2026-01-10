#pragma once
#include <ice/container/contiguous_container.hxx>
#include <ice/container/resizable_container.hxx>
#include <ice/mem_initializers.hxx>

namespace ice
{

    //! \brief A simple contaier storing items in contignous memory.
    //!
    //! \details Manages a memory block big enough to hold the items that it holds.
    //!
    //! \tparam Logic The logic used during memory operations for the given type.
    //!   This value cab be set by the user to enforce expected behavior for stored types.
    template<typename Type, ice::ContainerLogic Logic = ice::Constant_DefaultContainerLogic<Type>>
    struct Array
        : public ice::container::ContiguousContainer
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

        ice::Allocator* _allocator;
        ice::u32 _capacity;
        ice::u32 _count;
        ValueType* _data;

        inline explicit Array(ice::Allocator& alloc) noexcept;
        inline Array(Array&& other) noexcept;
        inline Array(Array const& other) noexcept
            requires std::copy_constructible<Type>;
        inline ~Array() noexcept;

        inline Array(
            ice::Allocator& alloc,
            ice::Span<Type const> values
        ) noexcept requires std::copy_constructible<Type>;

        // API Requirements Of: Container and Resizable Container
        template<typename Self>
        constexpr auto data(this Self& self) noexcept -> ice::container::ValuePtr<Self> { return self._data; }
        constexpr auto size() const noexcept -> ice::ncount { return { _count, sizeof(ValueType) }; }

        // API Requirements Of: Resizable Container
        constexpr auto capacity() const noexcept -> ice::ncount { return { _capacity, sizeof(ValueType) }; }
        constexpr void set_capacity(ice::ncount new_capacity) noexcept;
        constexpr void resize(ice::ncount new_size) noexcept;
        constexpr void clear() noexcept;

        // API Manipulation
        template<typename ItemType = Type>
            requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
        inline void push_back(ItemType&& item) noexcept;

        // API Requirements Of: Data and Memory
        constexpr auto data_view(this Array const& self) noexcept -> ice::Data;
        constexpr auto memory_view(this Array& self) noexcept -> ice::Memory;

        inline auto operator=(Array&& other) noexcept -> Array&;
        inline auto operator=(Array const& other) noexcept -> Array&
            requires std::copy_constructible<Type>;

        inline operator ice::Span<Type>() noexcept;
        inline operator ice::Span<Type const>() const noexcept;
    };

    template<typename Type, ice::ContainerLogic Logic>
    auto data_view(ice::Array<Type, Logic> const& arr) noexcept -> ice::Data
    {
        return arr.data_view();
    }

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
    { }

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
            set_capacity(other.size());

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    memory_view(),
                    other.data(),
                    other.size()
                );
            }
            else
            {
                ice::memcpy(
                    memory_view(),
                    other.data_view()
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

        _allocator->deallocate(memory_view());
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
            set_capacity(values.size());

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at(
                    memory_view(),
                    values.data(),
                    values.size()
                );
            }
            else
            {
                ice::memcpy(memory_view(), values.data_view());
            }

            _count = values.size().u32();
        }
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline auto Array<Type, Logic>::operator=(Array&& other) noexcept -> Array&
    {
        if (this != &other)
        {
            set_capacity(0);

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
            this->clear();
            this->reserve(other.capacity());

            if (other.size() > 0)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::mem_copy_construct_n_at(
                        memory_view(),
                        other.data(),
                        other.size()
                    );
                }
                else
                {
                    ice::memcpy(
                        memory_view(),
                        other.data_view()
                    );
                }
            }

            _count = other._count;
        }
        return *this;
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

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Array<Type, Logic>::set_capacity(ice::ncount new_capacity) noexcept
    {
        if (new_capacity == _capacity)
        {
            return;
        }

        if (new_capacity < _count)
        {
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_n_at(_data + new_capacity, _count - new_capacity);
            }

            _count = new_capacity.u32();
        }

        ValueType* new_data = nullptr;
        if (new_capacity > 0)
        {
            ice::AllocResult new_buffer = _allocator->allocate(ice::meminfo_of<ValueType> * new_capacity);
            ICE_ASSERT_CORE(new_buffer.memory != nullptr);
            if (_count > 0)
            {
                if constexpr (Logic == ContainerLogic::Complex)
                {
                    ice::mem_move_construct_n_at(new_buffer, _data, _count);
                    ice::mem_destruct_n_at(_data, _count);
                }
                else
                {
                    ice::memcpy(new_buffer, data_view());
                }
            }
            new_data = reinterpret_cast<ValueType*>(new_buffer.memory);
        }

        _allocator->deallocate(memory_view());
        _data = new_data;
        _capacity = new_capacity.u32();
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Array<Type, Logic>::resize(ice::ncount new_size) noexcept
    {
        if (_capacity < new_size)
        {
            set_capacity(new_size);
        }

        if (new_size > _count)
        {
            ice::ncount const missing_items = new_size - _count;
            ice::Memory const uninitialized_memory = ice::ptr_add(memory_view(), size());

            ice::mem_construct_n_at<ValueType>(
                uninitialized_memory,
                missing_items
            );
        }
        else if constexpr (Logic == ContainerLogic::Complex)
        {
            static_assert(Logic != ContainerLogic::Trivial);
            ice::ncount const destroyed_items = _count - new_size;

            ice::mem_destruct_n_at(
                _data + new_size,
                destroyed_items
            );
        }

        _count = new_size.u32();
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr void ice::Array<Type, Logic>::clear() noexcept
    {
        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_destruct_n_at(_data, _count);
        }
        _count = 0;
    }

    template<typename Type, ice::ContainerLogic Logic>
    template<typename ItemType>
        requires std::convertible_to<ItemType, Type> && std::is_constructible_v<Type, ItemType>
    inline void Array<Type, Logic>::push_back(ItemType&& item) noexcept
    {
        if (size() == capacity())
        {
            this->grow();
        }

        if constexpr (Logic == ContainerLogic::Complex)
        {
            ice::mem_construct_at<Type>(this->end(), ice::forward<ItemType>(item));
        }
        else
        {
            _data[_count] = Type{ ice::forward<ItemType>(item) };
        }

        _count += 1;
    }

    //template<typename Type, ice::ContainerLogic Logic, typename Value>
    //    requires std::copy_constructible<Type>&& std::convertible_to<Value, Type>
    //inline void push_back(ice::Array<Type, Logic>& arr, Value const& item) noexcept
    //{
    //    if (arr.size() == arr.capacity())
    //    {
    //        arr.grow();
    //    }

    //    if constexpr (Logic == ContainerLogic::Complex)
    //    {
    //        ice::mem_copy_construct_at<Type>(
    //            ice::ptr_add(arr.memory_view(), arr.size()),
    //            item
    //        );
    //    }
    //    else
    //    {
    //        arr._data[arr._count] = Type{ item };
    //    }

    //    arr._count += 1;
    //}

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto Array<Type, Logic>::data_view(this Array const& self) noexcept -> ice::Data
    {
        return ice::Data{
            .location = self.data(),
            .size = self.size(),
            .alignment = ice::align_of<ValueType>
        };
    }

    template<typename Type, ice::ContainerLogic Logic>
    inline constexpr auto Array<Type, Logic>::memory_view(this Array& self) noexcept -> ice::Memory
    {
        return ice::Memory{
            .location = self.data(),
            .size = self.capacity(),
            .alignment = ice::align_of<ValueType>
        };
    }


} // namespace ice
