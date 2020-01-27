#pragma once
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanCommandBuffers
    {
        VkCommandPool pool;
        VkCommandBuffer primary_buffers[2];
    };

    void allocate_command_buffers(
        VulkanDevices devices,
        VulkanCommandBuffers& command_buffers,
        uint32_t secondary_buffer_count,
        core::pod::Array<VkCommandBuffer>& secondary_buffers
    ) noexcept;

    void release_command_buffers(
        VulkanDevices devices,
        VulkanCommandBuffers command_buffers,
        core::pod::Array<VkCommandBuffer>& secondary_buffers
    ) noexcept;

} // namespace iceshard::renderer::vulkan
