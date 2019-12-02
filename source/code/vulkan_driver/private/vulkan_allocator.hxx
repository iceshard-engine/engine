#pragma once
#include <core/allocator.hxx>

namespace render::vulkan
{


    //! \brief A special allocator for vulkan allocations.
    class VulkanAllocator : public core::allocator
    {
    public:
        VulkanAllocator(core::allocator& backing_allocator) noexcept;
        ~VulkanAllocator() noexcept override;

        //! \copydoc allocator::allocate(uint32_t, uint32_t) noexcept
        auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void* override;

        //! \brief #todo
        auto reallocate(void* ptr, uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void*;

        //! \copydoc allocator::deallocate(void*) noexcept
        void deallocate(void* ptr) noexcept override;

        //! \copydoc allocator::allocated_size(void*) noexcept
        auto allocated_size(void* ptr) noexcept -> uint32_t override;

        //! \copydoc allocator::total_allocated() noexcept
        auto total_allocated() noexcept -> uint32_t override;

    private:
        core::allocator& _backing_allocator;

        uint32_t _total_allocated{ 0 };
    };


} // namespace iceshard::vulkan
