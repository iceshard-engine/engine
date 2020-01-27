#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>

namespace iceshard::renderer::vulkan
{

    void allocate_command_buffers(
        VulkanDevices devices,
        VulkanCommandBuffers& command_buffers,
        uint32_t secondary_buffer_count,
        core::pod::Array<VkCommandBuffer>& secondary_buffers
    ) noexcept
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = devices.graphics.family_index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        auto api_result = vkCreateCommandPool(devices.graphics.handle, &cmd_pool_info, NULL, &command_buffers.pool);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create command pool for device!");

        {
            VkCommandBufferAllocateInfo primary_alloc_info{ };
            primary_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            primary_alloc_info.pNext = nullptr;
            primary_alloc_info.commandPool = command_buffers.pool;
            primary_alloc_info.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            primary_alloc_info.commandBufferCount = core::size(command_buffers.primary_buffers);

            api_result = vkAllocateCommandBuffers(devices.graphics.handle, &primary_alloc_info, command_buffers.primary_buffers);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to allocate primary command buffers!");
        }

        if (secondary_buffer_count > 0)
        {
            core::pod::array::resize(secondary_buffers, secondary_buffer_count);

            VkCommandBufferAllocateInfo secondary_alloc_info{ };
            secondary_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            secondary_alloc_info.pNext = nullptr;
            secondary_alloc_info.commandPool = command_buffers.pool;
            secondary_alloc_info.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            secondary_alloc_info.commandBufferCount = core::pod::array::size(secondary_buffers);

            api_result = vkAllocateCommandBuffers(devices.graphics.handle, &secondary_alloc_info, core::pod::array::begin(secondary_buffers));
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to allocate secondary command buffers!");
        }
    }

    void release_command_buffers(
        VulkanDevices devices,
        VulkanCommandBuffers command_buffers,
        core::pod::Array<VkCommandBuffer>& secondary_buffers
    ) noexcept
    {
        core::pod::array::clear(secondary_buffers);
        vkDestroyCommandPool(devices.graphics.handle, command_buffers.pool, nullptr);
    }

} // namespace iceshard::renderer::vulkan