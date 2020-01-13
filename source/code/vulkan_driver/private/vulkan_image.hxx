#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#include "device/vulkan_physical_device.hxx"
#include "vulkan_device_memory_manager.hxx"
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanImage final
    {
    public:
        VulkanImage(VkDevice _device_handle, VkImage image, VkImageView image_view, VulkanMemoryInfo memory_info) noexcept;
        ~VulkanImage() noexcept;

        auto native_handle() const noexcept -> VkImage { return _image; }

        auto native_view() const noexcept -> VkImageView { return _image_view; }

    private:
        VkDevice const _device_handle;
        VkImage const _image;
        VkImageView const _image_view;
        VulkanMemoryInfo const _image_memory;
    };

    auto create_depth_buffer_image(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        VkExtent2D extent
    ) noexcept -> core::memory::unique_pointer<VulkanImage>;

    auto create_texture_2d(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        VkExtent2D extent
    ) noexcept -> core::memory::unique_pointer<VulkanImage>;

} // namespace render::vulkan
