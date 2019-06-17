#pragma once
#include <memsys/allocator.h>

namespace pod
{
    template<typename T>
    struct Array
    {
        Array(mem::allocator &a);
        Array(const Array &other);
        Array& operator=(const Array &other);
        ~Array();

        T& operator[](uint32_t i);
        const T& operator[](uint32_t i) const;

        mem::allocator* _allocator;
        uint32_t _size;
        uint32_t _capacity;
        T* _data;
    };

    /// A double-ended queue/ring buffer.
    template <typename T> struct Queue
    {
        Queue(mem::allocator &a);

        T &operator[](uint32_t i);
        const T &operator[](uint32_t i) const;

        Array<T> _data;
        uint32_t _size;
        uint32_t _offset;
    };

    /// Hash from an uint64_t to POD objects. If you want to use a generic key
    /// object, use a hash function to map that object to an uint64_t.
    template<typename T> struct Hash
    {
    public:
        Hash(mem::allocator &a);

        struct Entry {
            uint64_t key;
            uint32_t next;
            T value;
        };

        Array<uint32_t> _hash;
        Array<Entry> _data;
    };
}