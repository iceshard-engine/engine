#pragma once
#include "vk_include.hxx"
#include <ice/allocator.hxx>
#include <ice/map.hxx>

namespace ice::render::vk
{

    //! \brief A special allocator for vulkan allocations.
    class VulkanAllocator final : public ice::Allocator
    {
    public:
        VulkanAllocator(ice::Allocator& backing_allocator) noexcept;
        ~VulkanAllocator() noexcept override;

        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        auto reallocate(void* ptr, uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void*;

        void deallocate(void* ptr) noexcept override;

        auto allocated_size(void* ptr) const noexcept -> uint32_t override;

        auto total_allocated() const noexcept -> uint32_t override;

        auto vulkan_callbacks() const noexcept -> VkAllocationCallbacks const*;

        auto backing_allocator() const noexcept -> ice::Allocator&;

    private:
        ice::Allocator& _backing_allocator;
        ice::Map<void*, ice::u32> _allocation_tracker;
        ice::u32 _total_allocated{ 0 };

        VkAllocationCallbacks _vulkan_callbacks;
    };

    //auto allocation_callbacks_struct(VulkanAllocator& alloc) noexcept -> VkAllocationCallbacks;

} // namespace ice::render::vk
