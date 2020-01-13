#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/data/chunk.hxx>

#include <render_system/render_command_buffer.hxx>

#include "vulkan_device_memory_manager.hxx"
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanBuffer final
    {
    public:
        VulkanBuffer(VkBuffer buffer_handle, VulkanDeviceMemoryManager& memory_manager, VulkanMemoryInfo memory_info) noexcept;
        ~VulkanBuffer() noexcept;

        auto native_handle() const noexcept -> VkBuffer { return _buffer_handle; }

        void map_memory(render::api::BufferDataView& data_view) noexcept;

        void unmap_memory() noexcept;

    private:
        VkBuffer _buffer_handle;
        VulkanDeviceMemoryManager* _device_memory;
        VulkanMemoryInfo _memory_info;
    };

    auto create_staging_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>;

    auto create_uniform_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>;

    auto create_vertex_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>;

} // namespace render::vulkan
