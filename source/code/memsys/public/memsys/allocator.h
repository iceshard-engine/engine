#pragma once
#include <memsys/memsys_api.h>
#include <cinttypes>
#include <utility>

namespace mem
{
    /// Base class for memory allocators.
    ///
    /// Note: Regardless of which allocator is used, prefer to allocate memory in larger chunks
    /// instead of in many small allocations. This helps with data locality, fragmentation,
    /// memory usage tracking, etc.
    class MEMSYS_API allocator
    {
    public:
        static constexpr uint32_t SIZE_NOT_TRACKED = 0xffffffffu;

        /// Default alignment for memory allocations.
        static constexpr uint32_t DEFAULT_ALIGN = 4u;

        allocator() = default;
        virtual ~allocator() = default;

        /// Allocates the specified amount of memory aligned to the specified alignment.
        virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) = 0;

        /// Frees an allocation previously made with allocate().
        virtual void deallocate(void* ptr) = 0;

        /// Returns the amount of usable memory allocated at p. p must be a pointer
        /// returned by allocate() that has not yet been deallocated. The value returned
        /// will be at least the size specified to allocate(), but it can be bigger.
        /// (The allocator may round up the allocation to fit into a set of predefined
        /// slot sizes.)
        ///
        /// Not all allocators support tracking the size of individual allocations.
        /// An allocator that doesn't support it will return SIZE_NOT_TRACKED.
        virtual uint32_t allocated_size(void* ptr) = 0;

        /// Returns the total amount of memory allocated by this allocator. Note that the
        /// size returned can be bigger than the size of all individual allocations made,
        /// because the allocator may keep additional structures.
        ///
        /// If the allocator doesn't track memory, this function returns SIZE_NOT_TRACKED.
        virtual uint32_t total_allocated() = 0;

        /// Allocators cannot be copied.
        allocator(const allocator& other) = delete;
        allocator& operator=(const allocator& other) = delete;

    public:
        /// Allocates and constructs the given object using the arguments provided
        template<class T, class... Args>
        T* make(Args&&... args)
        {
            return new (allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
        }

        /// Destroys the given object and deallocates the memory used
        template<class T>
        void destroy(T* ptr)
        {
            if (ptr)
            {
                ptr->~T();
                deallocate(ptr);
            }
        }

        /// Destroys the given object and deallocates the memory used
        template<class T>
        void destroy(void* ptr)
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
