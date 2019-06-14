#pragma once
#include <memsys/allocator.hxx>

namespace memsys
{


//! \brief Allocates memory without reusing it later.
//!
//! \details This allocator should be use used when allocating simple objects in a single task,
//!     so the memory can be disposed after the work is done.
//! \details This allocator uses buckets to allocate more memory once and don't ask the backing allocator everytime a object is allocated.
class MEMSYS_API forward_allocator : public allocator
{
public:
    //! \brief Creates a forward allocator with the given bucket size.
    forward_allocator(memsys::allocator& backing, unsigned bucket_size) noexcept;

    //! \brief Releases all allocated memory.
    ~forward_allocator() noexcept override;

    //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
    auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void* override;

    //! \copydoc allocator::deallocate(void* ptr)
    void deallocate(void* ptr) noexcept override;

    //! \copydoc allocator::allocated_size(void* ptr)
    auto allocated_size(void* ptr) noexcept -> uint32_t override;

    //! \copydoc allocator::total_allocated
    auto total_allocated() noexcept -> uint32_t override;

    //! \brief Releaes all allocated memory.
    void release_all() noexcept;

protected:
    struct memory_bucket;

    //! \brief Allocates a new bucket with the requested size and alignment on the backing allocator.
    auto allocate_bucket(uint32_t size) noexcept -> memory_bucket*;

private:
    //! \brief The backing allocator.
    memsys::allocator& _backing;

    //! \brief Memory bucket list.
    memory_bucket* _bucket_list;

    //! \brief The bucket size.
    const unsigned _bucket_size;
};


} // namespace memsys
