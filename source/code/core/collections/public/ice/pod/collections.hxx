#pragma once
#include <ice/span.hxx>
#include <ice/allocator.hxx>

namespace ice::pod
{

    template<typename T>
    struct Array final
    {
        static_assert(
            ::std::is_trivially_copyable_v<T>,
            "ice::pod::Hash only accepts trivially copyable objects."
        );

        using ValueType = T;
        using Iterator = T*;
        using ReverSeIterator = std::reverse_iterator<T*>;
        using ConstIterator = T const*;
        using ConstReverseIterator = std::reverse_iterator<T const*>;

        explicit Array(ice::Allocator& alloc) noexcept;
        Array(Array&& other) noexcept;
        Array(Array const& other) noexcept;
        ~Array() noexcept;

        auto operator=(Array&& other) noexcept -> Array&;
        auto operator=(Array const& other) noexcept -> Array&;

        auto operator[](uint32_t i) noexcept -> T&;
        auto operator[](uint32_t i) const noexcept -> T const&;

        operator ice::Span<T>() noexcept;
        operator ice::Span<T const>() const noexcept;

        ice::Allocator* _allocator;
        uint32_t _size = 0;
        uint32_t _capacity = 0;
        T* _data = nullptr;
    };

    template<typename T>
    struct Queue final
    {
        static_assert(
            ::std::is_trivially_copyable_v<T>,
            "ice::pod::Hash only accepts trivially copyable objects."
        );

        using ValueType = T;

        explicit Queue(ice::Allocator& alloc) noexcept;
        ~Queue() noexcept = default;

        auto operator[](uint32_t i) noexcept -> T&;
        auto operator[](uint32_t i) const noexcept -> T const&;

        Array<T> _data;
        uint32_t _size{ 0 };
        uint32_t _offset{ 0 };
    };

    template<typename T>
    struct Hash final
    {
        static_assert(
            ::std::is_trivially_copyable_v<T>,
            "ice::pod::Hash only accepts trivially copyable objects."
        );

        struct Entry
        {
            uint64_t key;
            uint32_t next;
            T value;
        };

        using ValueType = T;
        using ConstIterator = typename ice::pod::Array<Entry>::ConstIterator;

        explicit Hash(ice::Allocator& alloc) noexcept;
        Hash(Hash&& other) noexcept;
        ~Hash() noexcept = default;

        auto operator=(Hash&& other) noexcept -> Hash&;

        operator ice::Span<Entry>() noexcept;
        operator ice::Span<Entry const>() const noexcept;

        Array<uint32_t> _hash;
        Array<Entry> _data;
    };

} // namespace ice::pod
