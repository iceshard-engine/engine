#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/data/chunk.hxx>

#include <render_system/render_command_buffer.hxx>

#include "device/vulkan_physical_device.hxx"
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanBuffer final
    {
    public:
        VulkanBuffer(VkDevice _device_handle, VkBuffer buffer_handle, VkDeviceMemory memory_handle) noexcept;
        ~VulkanBuffer() noexcept;

        auto native_handle() const noexcept -> VkBuffer { return _buffer_handle; }

        void map_memory(BufferDataView& data_view) noexcept;

        void unmap_memory() noexcept;

    private:
        VkDevice _device_handle;
        VkBuffer _buffer_handle;
        VkDeviceMemory _memory_handle;
    };

    auto create_uniform_buffer(
        core::allocator& alloc,
        VulkanPhysicalDevice* physical_device,
        VkDeviceSize buffer_size) noexcept -> core::memory::unique_pointer<VulkanBuffer>;

    auto create_vertex_buffer(
        core::allocator& alloc,
        VulkanPhysicalDevice* physical_device,
        VkDeviceSize buffer_size) noexcept -> core::memory::unique_pointer<VulkanBuffer>;

} // namespace render::vulkan
