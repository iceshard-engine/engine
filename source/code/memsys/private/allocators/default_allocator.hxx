#pragma once
#include <core/allocator.hxx>
#include "allocator_utils.hxx"
#include <atomic>

namespace core::memory
{

//! \brief An allocator that uses the default system malloc().
//! \details Allocations are padded so that we can store the size of each
//!     allocation and align them to the desired alignment.
//!
//! \remarks An OS-specific allocator that can do alignment and tracks size
//!     does need this padding and can thus be more efficient than the MallocAllocator.
class default_allocator : public core::allocator
{
public:
    default_allocator() noexcept = default;
    ~default_allocator() noexcept override;

    //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
    auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void* override;

    //! \copydoc allocator::deallocate(void* ptr)
    void deallocate(void* ptr) noexcept override;

    //! \copydoc allocator::allocated_size(void* ptr)
    auto allocated_size(void* ptr) noexcept -> uint32_t override;

    //! \copydoc allocator::total_allocated
    auto total_allocated() noexcept -> uint32_t override;

private:
    std::atomic_uint32_t _total_allocated{ 0 };
};


} // namespace core::memory
