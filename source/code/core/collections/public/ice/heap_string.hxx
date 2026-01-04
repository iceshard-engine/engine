/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string_concepts.hxx>
#include <ice/string/resizable_operations.hxx>
#include <ice/string.hxx>

namespace ice
{

    template<typename CharT = char> requires ice::concepts::SupportedCharType<CharT>
    struct HeapString : ice::string::ResizableOperations
    {
        using CharType = CharT;
        using ValueType = CharType;
        using Iterator = CharType*;
        using ReverseIterator = std::reverse_iterator<CharType*>;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;
        using SizeType = ice::ncount;
        using StringType = ice::BasicString<CharType>;

        ice::Allocator* _allocator;
        ice::u32 _capacity;
        ice::u32 _size;
        ValueType* _data;

        inline explicit HeapString(ice::Allocator& allocator) noexcept;
        inline HeapString(ice::Allocator& allocator, ice::BasicString<CharType> string) noexcept;
        inline ~HeapString() noexcept;

        inline HeapString(HeapString&& other) noexcept;
        inline HeapString(HeapString const& other) noexcept;

        inline auto operator=(HeapString&& other) noexcept -> HeapString&;
        inline auto operator=(HeapString const& other) noexcept -> HeapString&;
        inline auto operator=(ice::BasicString<CharType> str) noexcept -> HeapString&;

        inline operator ice::BasicString<CharType>() const noexcept;

        template<typename Self>
        inline auto data(this Self& self) noexcept -> ValueType* { return self._data; }

        inline auto size() const noexcept -> SizeType { return SizeType{ _size, sizeof(CharType) }; }
        inline void resize(ice::ncount new_size) noexcept;

        inline auto capacity() const noexcept -> SizeType { return SizeType{ _capacity, sizeof(CharType) }; }
        inline void set_capacity(ice::ncount new_capacity) noexcept;

        inline auto data_view() const noexcept -> ice::Data;
        inline auto extract_memory() noexcept -> ice::Memory;
    };

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::HeapString(ice::Allocator& allocator) noexcept
        : _allocator{ &allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::HeapString(ice::Allocator& allocator, ice::BasicString<CharT> value) noexcept
        : _allocator{ &allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
        *this = value;
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::HeapString(HeapString<CharT>&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ ice::exchange(other._size, 0) }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::HeapString(HeapString<CharT> const& other) noexcept
        : _allocator{ other._allocator }
        , _capacity{ 0 }
        , _size{ 0 }
        , _data{ nullptr }
    {
        if (other._size > 0)
        {
            set_capacity(other.size() + 1);

            // TODO: We need actually very, VERY good tests for string manipulations...
            ice::memcpy(
                this->end(),
                other.cbegin(),
                other.size().bytes()
            );

            _size = other._size;
            _data[_size] = CharType{ 0 };
        }
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::~HeapString() noexcept
    {
        _allocator->deallocate(this->memory_view());
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline auto HeapString<CharT>::operator=(HeapString<CharT>&& other) noexcept -> HeapString<CharT>&
    {
        if (this != ice::addressof(other))
        {
            set_capacity(0);

            _allocator = other._allocator;
            _size = ice::exchange(other._size, 0);
            _capacity = ice::exchange(other._capacity, 0);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline auto HeapString<CharT>::operator=(HeapString<CharT> const& other) noexcept -> HeapString<CharT>&
    {
        if (this != ice::addressof(other))
        {
            this->clear();
            this->reserve(other.capacity());

            if (other._size > 0)
            {
                ice::memcpy(
                    this->memory_view(),
                    other.data_view()
                );
            }

            _size = other._size;
            _data[_size] = CharType{ };
        }
        return *this;
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline auto HeapString<CharT>::operator=(ice::BasicString<CharT> str) noexcept -> HeapString<CharT>&
    {
        auto const* const other_str_begin = str.begin();
        bool const part_of_this = other_str_begin >= this->begin()
            && other_str_begin < this->end();
        ICE_ASSERT_CORE(part_of_this == false);

        if (!part_of_this)
        {
            set_capacity(str.size() + 1);
            ice::memcpy(
                this->memory_view(),
                str.data_view()
            );

            _size = str.size().u32();
            _data[_size] = ValueType{ 0 };
        }
        return *this;
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline HeapString<CharT>::operator ice::BasicString<typename HeapString<CharT>::CharType>() const noexcept
    {
        return ice::BasicString<CharT>{ _data, _size };
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline void HeapString<CharT>::resize(ice::ncount new_size) noexcept
    {
        if (new_size > 0 && new_size >= capacity())
        {
            set_capacity(new_size + 1);
        }

        _size = new_size.u32();
        if (_data != nullptr)
        {
            _data[_size] = CharType{ 0 };
        }
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline void HeapString<CharT>::set_capacity(ice::ncount new_capacity) noexcept
    {
        ice::u32 const new_capacity_u32 = new_capacity.u32();
        if (new_capacity_u32 == _capacity)
        {
            return;
        }

        ValueType* new_data = nullptr;
        if (new_capacity_u32 > 0)
        {
            ice::AllocResult new_buffer = _allocator->allocate(ice::meminfo_of<ValueType> * new_capacity);

            if (_size > 0)
            {
                ice::memcpy(new_buffer, this->data_view());
            }

            new_data = reinterpret_cast<ValueType*>(new_buffer.memory);
        }

        _allocator->deallocate(this->memory_view());
        _capacity = new_capacity_u32;
        _data = new_data;

        if (new_capacity_u32 <= _size)
        {
            _size = ice::min(new_capacity_u32, new_capacity_u32 - 1);
        }

        if (_data != nullptr)
        {
            _data[_size] = 0;
        }
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline auto HeapString<CharT>::data_view() const noexcept -> ice::Data
    {
        return Data{
            .location = _data,
            .size = size().bytes(),
            .alignment = ice::align_of<CharType>
        };
    }

    template<typename CharT> requires ice::concepts::SupportedCharType<CharT>
    inline auto HeapString<CharT>::extract_memory() noexcept -> ice::Memory
    {
        _size = 0; // clear the size too
        return {
            ice::exchange(_data, nullptr),
            ice::exchange(_capacity, 0),
            ice::align_of<CharType>
        };
    }

} // namespace ice
