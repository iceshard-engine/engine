#pragma once
#include <core/collections.hxx>
#include <render_system/render_api.hxx>
#include "vulkan_allocator.hxx"

#include <iceshard/renderer/vulkan/vulkan_devices.hxx>

namespace iceshard::renderer::vulkan
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
        VulkanDeviceMemoryManager(core::allocator& alloc, iceshard::renderer::vulkan::VulkanDevices devices) noexcept;
        ~VulkanDeviceMemoryManager() noexcept;

        auto graphics_device() noexcept -> VkDevice { return _devices.graphics.handle; }

        bool allocate_memory(VkBuffer buffer, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept;
        bool allocate_memory(VkImage image, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept;

        void deallocate_memory(VkBuffer buffer, VulkanMemoryInfo const& memory_info) noexcept;
        void deallocate_memory(VkImage image, VulkanMemoryInfo const& memory_info) noexcept;

        void map_memory(VulkanMemoryInfo* ranges, api::DataView* views, uint32_t size) noexcept;

        void unmap_memory(VulkanMemoryInfo* ranges, uint32_t size);

    protected:
        void allocate_memory(uint32_t memory_type, VkDeviceSize size, VkDeviceSize alignment, VulkanMemoryInfo& memory_info) noexcept;

    private:
        render::vulkan::VulkanAllocator _vulkan_allocator;

        iceshard::renderer::vulkan::VulkanDevices _devices;

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

