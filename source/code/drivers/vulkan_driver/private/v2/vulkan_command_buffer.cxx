#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanCommandBufferPool::VulkanCommandBufferPool(VulkanDevices devices) noexcept
        : _graphics_device{ devices.graphics.handle }
        , _command_buffer_pool{ vk_nullptr }
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = devices.graphics.family_index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        auto api_result = vkCreateCommandPool(_graphics_device, &cmd_pool_info, NULL, &_command_buffer_pool);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create command pool for device!");
    }

    VulkanCommandBufferPool::~VulkanCommandBufferPool() noexcept
    {
        vkDestroyCommandPool(_graphics_device, _command_buffer_pool, nullptr);
    }

    void VulkanCommandBufferPool::allocate_buffers(
        api::CommandBufferType type,
        core::pod::Array<VkCommandBuffer>& buffers
    ) noexcept
    {
        VkCommandBufferAllocateInfo alloc_info{ };
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.commandPool = _command_buffer_pool;
        alloc_info.commandBufferCount = core::pod::array::size(buffers);

        if (type == api::CommandBufferType::Primary)
        {
            alloc_info.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }
        else
        {
            alloc_info.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        }

        //auto* const buffers_ptr = reinterpret_cast<VkCommandBuffer*>();
        auto api_result = vkAllocateCommandBuffers(_graphics_device, &alloc_info, core::pod::array::begin(buffers));
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to allocate primary command buffers!");
    }

} // namespace iceshard::renderer::vulkan