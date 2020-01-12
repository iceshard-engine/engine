#pragma once
#include <core/collections.hxx>
#include "device/vulkan_physical_device.hxx"
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    struct VulkanMemoryInfo
    {
        VkDeviceMemory memory_handle;
        uint32_t memory_offset;
        uint32_t memory_size;
    };

    class VulkanDeviceMemoryManager
    {
    public:
        VulkanDeviceMemoryManager(core::allocator& alloc, VulkanPhysicalDevice const* physical_device, VkDevice graphics_device) noexcept;
        ~VulkanDeviceMemoryManager() noexcept;

        auto graphics_device() noexcept -> VkDevice { return _graphics_device; }

        bool allocate_memory(VkBuffer buffer, VulkanMemoryInfo& memory_info) noexcept;

    private:
        VulkanPhysicalDevice const* _physical_device;
        VkDevice const _graphics_device;

        struct DeviceMemoryBlock
        {
            VkDeviceMemory const device_memory_handle;
            uint32_t const device_memory_total;
            uint32_t device_memory_usage = 0;
        };

        core::Map<uint32_t, DeviceMemoryBlock> _buffer_allocators;
    };

} // namespace render::vulkan

