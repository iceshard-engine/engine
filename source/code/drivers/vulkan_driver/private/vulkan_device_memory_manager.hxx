#pragma once
#include <core/collections.hxx>
#include <render_system/render_api.hxx>
#include "device/vulkan_physical_device.hxx"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
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

        bool allocate_memory(VkBuffer buffer, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept;

        bool allocate_memory(VkImage image, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept;

        void map_memory(VulkanMemoryInfo* ranges, render::api::BufferDataView* views, uint32_t size) noexcept;

        void unmap_memory(VulkanMemoryInfo* ranges, uint32_t size);

    protected:
        void allocate_memory(uint32_t memory_type, VkDeviceSize size, VkDeviceSize alignment, VulkanMemoryInfo& memory_info) noexcept;

    private:
        VulkanPhysicalDevice const* _physical_device;
        VkDevice const _graphics_device;

        struct DeviceMemoryBlock
        {
            VkDeviceMemory const device_memory_handle;
            VkDeviceSize const device_memory_total;
            VkDeviceSize device_memory_usage = 0;
        };

        core::Vector<DeviceMemoryBlock> _memory_blocks;
        core::Map<VkDeviceMemory, uint32_t> _block_info;
        core::Map<uint32_t, uint32_t> _current_memory_blocks;
    };

} // namespace render::vulkan

