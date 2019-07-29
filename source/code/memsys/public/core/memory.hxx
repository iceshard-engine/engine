#pragma once
#include <core/allocator.hxx>
#include <type_traits>
#include <memory>

//! \brief The memory system namespace.
namespace core::memory
{


    //! \brief Functions for accessing global memory data.
    namespace globals
    {

        //! \brief Initializes the global memory allocators.
        //!
        //! \param [in] scratch_buffer_size The size of the memory buffer used by the scratch allocators.
        MEMSYS_API void init(uint32_t scratch_buffer_size = 4 * 1024 * 1024) noexcept;

        //! \brief Initializes the global memory allocators with statistics.
        //! \details A proxy allocator is inserted between the default allocator and used
        //!     to track additional allocation information.
        //!
        //! \param [in] scratch_buffer_size The size of the memory buffer used by the scratch allocators.
        MEMSYS_API void init_with_stats(uint32_t scratch_buffer_size = 4 * 1024 * 1024) noexcept;

        //! \brief Returns a default memory allocator that can be used for most allocations.
        //! \pre You need to call init() for this allocator to be available.
        MEMSYS_API auto default_allocator() noexcept -> core::allocator&;

        //! \brief Returns a "scratch" allocator that can be used for temporary short-lived memory allocations.
        //! \details The scratch allocator uses a ring buffer of size scratch_buffer_size to service the allocations.
        //!
        //! \remarks If there is not enough memory in the buffer to match requests for scratch memory,
        //!     memory from the default_allocator will be returned instead.
        MEMSYS_API auto default_scratch_allocator() noexcept -> core::allocator&;

        //! \brief A special allocator reference which always fails to allocate.
        MEMSYS_API auto null_allocator() noexcept -> core::allocator&;

        //! Shuts down the global memory allocators created by init().
        MEMSYS_API void shutdown() noexcept;

    } // namespace globals


    namespace utils
    {

        //! \brief Aligns the pointer to the specified alignment.
        //! \details If necessary the alignment is done by advancing the pointer.
        inline auto align_forward(void* ptr, uint32_t align) noexcept -> void*;

        //! \brief Aligns the pointer to the specified alignment.
        //! \details If necessary the alignment is done by advancing the pointer.
        inline auto align_forward(const void* ptr, uint32_t align) noexcept -> const void*;

        //! \brief Advances the pointer by the given number of bytes.
        inline auto pointer_add(void* ptr, uint32_t bytes) noexcept -> void*;

        //! \brief Advances the pointer forward by the given number of bytes.
        inline auto pointer_add(const void* ptr, uint32_t bytes) noexcept -> const void*;

        //! \brief Decreases the pointer by the given number of bytes.
        inline auto pointer_sub(void* ptr, uint32_t bytes) noexcept -> void*;

        //! \brief Decreases the pointer by the given number of bytes.
        inline auto pointer_sub(const void* ptr, uint32_t bytes) noexcept -> const void*;

        //! \brief Returns the distance in bytes between pointers.
        inline auto pointer_distance(void* from, void* to) noexcept -> int32_t;

        //! \brief Returns the distance between pointers in bytes.
        inline auto pointer_distance(const void* from, const void* to) noexcept -> int32_t;

    } // namespace utils


#include "memory.inl"


} // namespace core::memory
