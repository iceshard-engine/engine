#pragma once
#include <ice/allocator.hxx>
#include <string_view>
#include <string>

namespace ice::memory
{

    //! \brief This class proxies allocation request to the backing allocator.
    //! \details Additionally this allocator tracks the total allocated size.
    //!
    //! \pre Allocation tracking is only possible if the backing allocator supports it.
    //!
    //! \remarks Proxy allocators have a name which will be used in various debugging scenarios.
    class ProxyAllocator : public ice::Allocator
    {
    public:
        //! \brief Creates a new proxy allocator with the given name for the given backing allocator.
        ProxyAllocator(ice::Allocator& alloc, std::string_view name) noexcept;

        //! \brief Checks if all allocations were released if supported.
        ~ProxyAllocator() noexcept override;

        //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        //! \copydoc allocator::deallocate(void* ptr)
        void deallocate(void* ptr) noexcept override;

        //! \copydoc allocator::allocated_size(void* ptr)
        auto allocated_size(void* ptr) noexcept -> uint32_t override;

        //! \copydoc allocator::total_allocated
        auto total_allocated() noexcept -> uint32_t override;

        //! \brief The backing allocator.
        auto backing_allocator() noexcept -> ice::Allocator& { return _backing_allocator; }

    public:
        //! \brief Returns the total number of allocations.
        auto allocation_count() const noexcept -> uint32_t { return _allocation_requests; }

    private:
        //! \brief Backing allocator.
        ice::Allocator& _backing_allocator;

        //! \brief Name of the proxy allocator.
        std::string const _name;

        //! \brief True if the backing allocator supports allocation tracking.
        bool const _allocation_tracking;

        //! \brief Total allocated size.
        uint32_t _allocation_total{ 0 };

        //! \brief Number of allocations.
        uint32_t _allocation_requests{ 0 };
    };


} // namespace ice::memory
