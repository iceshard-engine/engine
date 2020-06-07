#pragma once
#include <iceshard/renderer/render_types.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    union VulkanCommandBuffer
    {
        CommandBuffer handle;
        VkCommandBuffer native;
    };

    class VulkanCommandBufferPool
    {
    public:
        VulkanCommandBufferPool(VulkanDevices devices) noexcept;
        ~VulkanCommandBufferPool() noexcept;

        void allocate_buffers(
            api::CommandBufferType type,
            core::pod::Array<VkCommandBuffer>& buffers
        ) noexcept;

    private:
        VkDevice _graphics_device;
        VkCommandPool _command_buffer_pool;
    };

} // namespace iceshard::renderer::vulkan
