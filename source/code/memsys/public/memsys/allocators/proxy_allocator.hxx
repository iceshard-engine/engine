#pragma once
#include <memsys/allocator.hxx>
#include <string_view>
#include <string>

namespace memsys
{


//! \brief This class proxies allocation request to the backing allocator.
//! \details Aditionally this allocator tracks the total allocated size.
//!
//! \pre Allocation tracking is only possible if the backing allocator supports it.
//!
//! \remarks Proxy allocators have a name which will be used in various debugging scenarios.
class MEMSYS_API proxy_allocator : public allocator
{
public:
    //! \brief Creates a new proxy allocator with the given name for the given backing allocator.
    proxy_allocator(std::string_view name, memsys::allocator& alloc) noexcept;

    //! \brief Checks if all allocations were released if supported.
    virtual ~proxy_allocator() noexcept override;

    //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
    auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void* override;

    //! \copydoc allocator::deallocate(void* ptr)
    void deallocate(void* ptr) noexcept override;

    //! \copydoc allocator::allocated_size(void* ptr)
    auto allocated_size(void* ptr) noexcept -> uint32_t override;

    //! \copydoc allocator::total_allocated
    auto total_allocated() noexcept -> uint32_t override;

    //! \brief The backing allocator.
    auto backing_allocator() noexcept -> allocator& { return _backing_allocator; }

private:
    //! \brief Name of the proxy allocator.
    const std::string _name;

    //! \brief Backing allocator.
    allocator& _backing_allocator;

    //! \brief True if the backing allocator supports allocation tracking.
    const bool _allocation_tracking;

    //! \brief Total allocated size.
    uint32_t _allocation_total{ 0 };
};


} // namespace memsys
