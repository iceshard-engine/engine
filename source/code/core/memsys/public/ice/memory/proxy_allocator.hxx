#pragma once
#include <ice/allocator.hxx>
#include <string_view>
#include <string>

namespace ice::memory
{

#if ICE_DEBUG || ICE_DEVELOP

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
        auto allocated_size(void* ptr) const noexcept -> uint32_t override;

        //! \copydoc allocator::total_allocated
        auto total_allocated() const noexcept -> uint32_t override;

        //! \brief The backing allocator.
        auto backing_allocator() noexcept -> ice::Allocator& { return _backing_allocator; }

        //! \brief Returns the total number of allocations.
        auto allocation_count() const noexcept -> uint32_t override { return _allocation_requests; }

    private:
        //! \brief Backing allocator.
        ice::Allocator& _backing_allocator;

        //! \brief True if the backing allocator supports allocation tracking.
        bool const _allocation_tracking;

        //! \brief Total allocated size.
        uint32_t _allocation_total{ 0 };

        //! \brief Number of allocations.
        uint32_t _allocation_requests{ 0 };
    };

#elif ICE_PROFILE

    class ProxyAllocator final : public ice::Allocator
    {
    public:
        ProxyAllocator(ice::Allocator& alloc, std::string_view name) noexcept
            : _name{ name }
            , _backing_allocator{ alloc }
        { }
        ~ProxyAllocator() noexcept override = default;

        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        void deallocate(void* ptr) noexcept override;

        inline auto allocated_size(void* ptr) const noexcept -> uint32_t override
        {
            return _backing_allocator.allocated_size(ptr);
        }

        inline auto total_allocated() const noexcept -> uint32_t override
        {
            return _backing_allocator.total_allocated();
        }

        inline auto backing_allocator() noexcept -> ice::Allocator&
        {
            return _backing_allocator;
        }

        auto allocation_count() const noexcept -> uint32_t
        {
            return Constant_SizeNotTracked;
        }

    private:
        std::string_view _name;
        ice::Allocator& _backing_allocator;
    };

#else // #if ICE_DEBUG || ICE_DEVELOP || ICE_PROFILE

    class ProxyAllocator final : public ice::Allocator
    {
    public:
        ProxyAllocator(ice::Allocator& alloc, std::string_view) noexcept
            : _backing_allocator{ alloc }
        { }
        ~ProxyAllocator() noexcept override = default;

        inline auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override
        {
            return _backing_allocator.allocate(size, align);
        }

        inline void deallocate(void* ptr) noexcept override
        {
            return _backing_allocator.deallocate(ptr);
        }

        inline auto allocated_size(void* ptr) const noexcept -> uint32_t override
        {
            return _backing_allocator.allocated_size(ptr);
        }

        inline auto total_allocated() const noexcept -> uint32_t override
        {
            return _backing_allocator.total_allocated();
        }

        inline auto backing_allocator() noexcept -> ice::Allocator&
        {
            return _backing_allocator;
        }

        auto allocation_count() const noexcept -> uint32_t
        {
            return Constant_SizeNotTracked;
        }

    private:
        ice::Allocator& _backing_allocator;
    };

#endif // #if ICE_DEBUG || ICE_DEVELOP

} // namespace ice::memory
