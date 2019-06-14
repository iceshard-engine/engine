#pragma once
#include <memsys/allocator.hxx>
#include <cassert>

namespace memsys
{


/// An allocator used to allocate temporary "scratch" memory. The allocator
/// uses a fixed size ring buffer to services the requests.
///
/// Memory is always always allocated linearly. An allocation pointer is
/// advanced through the buffer as memory is allocated and wraps around at
/// the end of the buffer. Similarly, a free pointer is advanced as memory
/// is freed.
///
/// It is important that the scratch allocator is only used for short-lived
/// memory allocations. A long lived allocator will lock the "free" pointer
/// and prevent the "allocate" pointer from proceeding past it, which means
/// the ring buffer can't be used.
///
/// If the ring buffer is exhausted, the scratch allocator will use its backing
/// allocator to allocate memory instead.
class MEMSYS_API scratch_allocator : public memsys::allocator
{
public:
    /// Creates a ScratchAllocator. The allocator will use the backing
    /// allocator to create the ring buffer and to service any requests
    /// that don't fit in the ring buffer.
    ///
    /// size specifies the size of the ring buffer.
    scratch_allocator(allocator &backing, uint32_t size) noexcept;

    virtual ~scratch_allocator() noexcept override;

    bool in_use(void* ptr) noexcept;

    virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept override;

    virtual void deallocate(void *p) noexcept override;

    virtual uint32_t allocated_size(void *p) noexcept override;

    virtual uint32_t total_allocated() noexcept override;

    virtual memsys::allocator& backing_allocator() noexcept;

private:
    // Start and end of the ring buffer.
    char *_begin, *_end;

    // Pointers to where to allocate memory and where to free memory.
    char *_allocate, *_free;

    // The backing allocator
    memsys::allocator &_backing;
};


} // namespace memsys
