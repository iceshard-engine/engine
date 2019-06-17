#pragma once
#include <memsys/allocator.hxx>

//! \brief Collections used to store Plain Old Data values.
//! \details These collections do not call constructors or destructors on those values.
namespace pod
{


//! \brief This type defines an dynamic array holding POD values in contiguous memory.
template<typename T>
struct Array final
{
    //! \brief Creates a new Array with the given allocator.
    Array(memsys::allocator& allocator) noexcept;
    ~Array();

    //! \brief Copies data from the given array.
    Array(const Array &other) noexcept;

    //! \brief Copies data from the given array and releases the current data.
    auto operator=(const Array &other) noexcept -> Array&;

    //! \brief Returns the object at the given index.
    auto operator[](uint32_t i) noexcept(false) -> T&;

    //! \brief Returns the object at the given index.
    auto operator[](uint32_t i) noexcept(false) const -> const T&;

    //! \brief The allocator used to manage memory.
    memsys::allocator* _allocator;

    //! \brief The current array size.
    uint32_t _size;

    //! \brief The current available array capacity.
    uint32_t _capacity;

    //! \brief The array data.
    T* _data;
};

//! \brief A double-ended queue/ring buffer.
template <typename T> struct Queue
{
    //! \brief Creates a new Queue with the given allocator.
    Queue(memsys::allocator& allocator) noexcept;

    //! \brief Returns the object at the given index.
    auto operator[](uint32_t i) noexcept(false) -> T&;

    //! \brief Returns the object at the given index.
    auto operator[](uint32_t i) noexcept(false) const -> const T&;

    //! \brief The Array object used for the ring buffer.
    Array<T> _data;

    //! \brief The current queue size.
    uint32_t _size;

    //! \brief The offset for the queue head.
    uint32_t _offset;
};

//! \brief Hash from an uint64_t to POD objects.
//! \details If you want to use a generic key object, use a hash function to map that object to an uint64_t.
template<typename T> struct Hash
{
public:
    //! \brief Creates a new Hash with the given allocator.
    Hash(memsys::allocator& allocator) noexcept;

    //! \brief The entry used to hold and manage hashed objects.
    struct Entry
    {
        uint64_t key;
        uint32_t next;
        T value;
    };

    //! \brief Array with hash keys.
    Array<uint32_t> _hash;

    //! \brief Array with hashed values.
    Array<Entry> _data;
};


} // namespace pod
