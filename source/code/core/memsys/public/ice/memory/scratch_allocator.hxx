#pragma once
#include <ice/allocator.hxx>

namespace ice::memory
{

    //! \brief An allocator used to allocate temporary "scratch" memory.
    //! \details The allocator uses a fixed size ring buffer to services the requests.
    //!     Memory is always always allocated linearly. An allocation pointer is
    //!     advanced through the buffer as memory is allocated and wraps around at
    //!     the end of the buffer. Similarly, a free pointer is advanced as memory
    //!     is freed.
    //!
    //! \remarks It is important that the scratch allocator is only used for short-lived
    //!     memory allocations. A long lived allocator will lock the "free" pointer
    //!     and prevent the "allocate" pointer from proceeding past it, which means
    //!     the ring buffer can't be used.
    //!
    //! \remarks If the ring buffer is exhausted, the scratch allocator will use its backing
    //!     allocator to allocate memory instead.
    class ScratchAllocator final : public ice::Allocator
    {
    public:
        //! \brief Creates a ScratchAllocator.
        //! \details The allocator will use the backing
        //!     allocator to create the ring buffer and to service any requests
        //!     that don't fit in the ring buffer.
        //!
        //! \param [in] backing Specifies the backing allocator.
        //! \param [in] size Specifies the size of the ring buffer.
        ScratchAllocator(
            ice::Allocator& backing,
            uint32_t size,
            std::string_view name = { }
        ) noexcept;

        //! \brief Checks the allocation status and releases the ring buffer.
        ~ScratchAllocator() noexcept override;

        //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        //! \copydoc allocator::deallocate(void* ptr)
        void deallocate(void* pointer) noexcept override;

        //! \copydoc allocator::allocated_size(void* ptr)
        auto allocated_size(void* pointer) const noexcept -> uint32_t override;

        //! \copydoc allocator::total_allocated
        auto total_allocated() const noexcept -> uint32_t override;

        //! \brief The backing allocator.
        auto backing_allocator() noexcept -> ice::Allocator& { return _backing; }

        //! \brief Resets the allocator forgetting about all allocations.
        bool reset_and_discard() noexcept;

    protected:
        //! \brief Checks if the given pointer is locked in the ring buffer.
        bool is_locked(void* pointer) noexcept;

        //! \brief Checks if this allocator owns this pointer
        bool is_backing_pointer(void* pointer) const noexcept;

    private:
        ice::Allocator& _backing;

        // Start and end of the ring buffer.
        void* _begin{ nullptr };
        void* _end{ nullptr };

        // Pointers to where to allocate memory and where to free memory.
        void* _allocate{ nullptr };
        void* _free{ nullptr };
    };


} // namespace core::memory
