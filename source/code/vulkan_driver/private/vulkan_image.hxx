#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#include "device/vulkan_physical_device.hxx"
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanImage final
    {
    public:
        VulkanImage(VkDevice _device_handle, VkImage image_handle, VkDeviceMemory memory_handle) noexcept;
        ~VulkanImage() noexcept;

        auto native_handle() const noexcept -> VkImage { return _image_handle; }

        auto native_view() const noexcept -> VkImageView { return _view_handle; }

    private:
        VkDevice _device_handle;
        VkImage _image_handle;
        VkDeviceMemory _memory_handle;
        VkImageView _view_handle = nullptr;
    };

    auto create_depth_buffer_image(core::allocator& alloc, VulkanPhysicalDevice* physical_device) noexcept -> core::memory::unique_pointer<VulkanImage>;

} // namespace render::vulkan
