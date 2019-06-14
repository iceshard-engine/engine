//!
//! This file was created based the 'BitSquid / Foundation' repository.
//! The original code can be found here: https://bitbucket.org/bitsquid/foundation/src/default/
//!
//! Some modifications were introduced but the idea is the same.
//!

#pragma once
#include <memsys/memsys_api.h>
#include <cinttypes>
#include <utility>

namespace memsys
{


//! \brief Base class for memory allocators.
//!
//! \details Regardless of which allocator is used, prefer to allocate memory in larger chunks
//!     instead of in many small allocations. This helps with data locality, fragmentation,
//!     memory usage tracking, etc.
class MEMSYS_API allocator
{
public:
    //! \brief Special constant value.
    static constexpr uint32_t SIZE_NOT_TRACKED = 0xffffffffu;

    //! \brief Default alignment for memory allocations.
    static constexpr uint32_t DEFAULT_ALIGN = 4u;

    allocator()  = default;
    virtual ~allocator()  = default;

    //! \brief Allocates the specified amount of memory aligned to the specified alignment.
    virtual auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN)  -> void*;

    //! \brief Frees an allocation previously made with allocate().
    virtual void deallocate(void* ptr)  = 0;

    //! \brief Returns the amount of usable memory allocated at ptr.
    //! \details The value returned will be at least the size specified to allocate(), but it can be bigger.
    //!     (The allocator may round up the allocation to fit into a set of predefined slot sizes.)
    //!
    //! \pre ptr must be a pointer returned by allocate() that has not yet been deallocated.
    //!
    //! \remarks Not all allocators support tracking the size of individual allocations.
    //!     An allocator that doesn't support it will return SIZE_NOT_TRACKED.
    virtual auto allocated_size(void* ptr)  -> uint32_t = 0;

    //! \brief Returns the total amount of memory allocated by this allocator.
    //!
    //! \remarks The size returned can be bigger than the size of all individual allocations made,
    //!     because the allocator may keep additional structures.
    //! \remarks If the allocator doesn't track memory, this function returns SIZE_NOT_TRACKED.
    virtual auto total_allocated()  -> uint32_t = 0;

    //! Allocators cannot be copied.
    allocator(const allocator& other)  = delete;

    //! Allocators cannot be copied.
    allocator& operator=(const allocator& other)  = delete;

public:
    //! Allocates and constructs the given object using the provided arguments.
    template<class T, class... Args>
    auto make(Args&&... args) noexcept -> T*
    {
        return new (allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
    }

    /// Destroys the given object and deallocates the memory used
    template<class T>
    void destroy(T* ptr) noexcept
    {
        if (ptr)
        {
            ptr->~T();
            deallocate(ptr);
        }
    }

    /// Destroys the given object and deallocates the memory used
    template<class T>
    void destroy(void* ptr) noexcept
    {
        if (ptr)
        {
            reinterpret_cast<T*>(ptr)->~T();
            deallocate(ptr);
        }
    }
};

/// Creates a new object of type T using the allocator `a` to allocate the memory.
#   define MAKE_NEW(a, T, ...)        (new ((a).allocate(sizeof(T), alignof(T))) T(__VA_ARGS__))

/// Frees an object allocated with MAKE_NEW.
#   define MAKE_DELETE(a, T, p)    do {if (p) {(p)->~T(); a.deallocate(p);}} while (0)

/// Creates a new array of objects of type T using the allocator `a` to allocate the memory. (no constructor is called for any element!)
#   define MAKE_NEW_ARRAY(a, T, size)   reinterpret_cast<T*>((a).allocate(sizeof(T) * size, alignof(T)))

/// Frees an array allocated with MAKE_NEW_ARRAY (no destructor is called for any element!)
#   define MAKE_DELETE_ARRAY(a, p) do {if (p) { a.deallocate(p); }} while (0)


}
