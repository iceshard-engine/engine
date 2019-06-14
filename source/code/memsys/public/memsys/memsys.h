#pragma once
#include <memsys/allocator.h>

#include <type_traits>
#include <memory>

namespace mem
{
    /// Functions for accessing global memory data.
    namespace globals
    {
        /// Initializes the global memory allocators. scratch_buffer_size is the size of the
        /// memory buffer used by the scratch allocators.
        MEMSYS_API void init(uint32_t scratch_buffer_size = 4 * 1024 * 1024);

        /// Returns a default memory allocator that can be used for most allocations.
        ///
        /// You need to call init() for this allocator to be available.
        MEMSYS_API allocator &default_allocator();

        /// Returns a "scratch" allocator that can be used for temporary short-lived memory
        /// allocations. The scratch allocator uses a ring buffer of size scratch_buffer_size
        /// to service the allocations.
        ///
        /// If there is not enough memory in the buffer to match requests for scratch
        /// memory, memory from the default_allocator will be returned instead.
        MEMSYS_API allocator &default_scratch_allocator();

        /// Shuts down the global memory allocators created by init().
        MEMSYS_API void shutdown();
    }

    namespace utils
    {
        inline void* align_forward(void* ptr, uint32_t align);
        inline void* pointer_add(void* ptr, uint32_t bytes);
        inline const void* pointer_add(const void* ptr, uint32_t bytes);
        inline void* pointer_sub(void* ptr, uint32_t bytes);
        inline const void* pointer_sub(const void* ptr, uint32_t bytes);
        inline int32_t pointer_distance(void* from, void* to);
        inline int32_t pointer_distance(const void* from, const void* to);
    }


    // ---------------------------------------------------------------
    // Inline function implementations
    // ---------------------------------------------------------------

    // Aligns p to the specified alignment by moving it forward if necessary
    // and returns the result.
    inline void* utils::align_forward(void* p, uint32_t align)
    {
        uintptr_t pi = uintptr_t(p);
        const uint32_t mod = pi % align;
        if (mod)
            pi += (align - mod);
        return reinterpret_cast<void*>(pi);
    }

    /// Returns the result of advancing p by the specified number of bytes
    inline void* utils::pointer_add(void* ptr, uint32_t bytes)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) + bytes);
    }

    inline const void* utils::pointer_add(const void* ptr, uint32_t bytes)
    {
        return reinterpret_cast<const void*>(reinterpret_cast<const char*>(ptr) + bytes);
    }

    /// Returns the result of moving p back by the specified number of bytes
    inline void* utils::pointer_sub(void* ptr, uint32_t bytes)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) - bytes);
    }

    inline const void* utils::pointer_sub(const void* ptr, uint32_t bytes)
    {
        return reinterpret_cast<const void*>(reinterpret_cast<const char*>(ptr) - bytes);
    }

    /// Returns the distance in bytes between two pointers
    inline int32_t utils::pointer_distance(void* from, void* to)
    {
        auto distance = reinterpret_cast<char*>(to) - reinterpret_cast<char*>(from);
        return static_cast<int32_t>(distance);
    }

    inline int32_t utils::pointer_distance(const void* from, const void* to)
    {
        return static_cast<int32_t>(reinterpret_cast<const char*>(to) - reinterpret_cast<const char*>(from));
    }
}
