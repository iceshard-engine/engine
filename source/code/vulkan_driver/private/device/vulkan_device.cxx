#include "vulkan_device.hxx"

#include <core/debug/assert.hxx>

namespace render::vulkan
{

    VulkanDevice::VulkanDevice(
        core::allocator& alloc,
        VulkanDeviceQueueType queue_type,
        VulkanQueueFamilyIndex queue_family,
        VkDevice device_handle) noexcept
        : _allocator{ alloc }
        , _queue_type{ queue_type }
        , _queue_family{ queue_family }
        , _device_handle{ device_handle }
        , _command_buffers{ _allocator }
    {
        initialize();
    }

    VulkanDevice::~VulkanDevice() noexcept
    {
        shutdown();
    }

    void VulkanDevice::create_command_buffers(core::pod::Array<VulkanCommandBuffer*>& output_array, uint32_t num) noexcept
    {
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = NULL;
        cmd.commandPool = _command_pool;
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = num;

        VkCommandBuffer command_buffer;
        auto api_result = vkAllocateCommandBuffers(_device_handle, &cmd, &command_buffer);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create command buffers! Requested count: {}", num);

        VulkanCommandBuffer* result = _allocator.make<VulkanCommandBuffer>(_allocator, command_buffer);
        core::pod::array::push_back(_command_buffers, result);
        core::pod::array::push_back(output_array, result);
    }

    void VulkanDevice::initialize() noexcept
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = static_cast<uint32_t>(_queue_family);
        cmd_pool_info.flags = 0;

        auto api_result = vkCreateCommandPool(_device_handle, &cmd_pool_info, NULL, &_command_pool);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create command pool for device!");
    }

    void VulkanDevice::shutdown() noexcept
    {
        for (auto* command_buffer : _command_buffers)
        {
            _allocator.destroy(command_buffer);
        }

        vkDestroyCommandPool(_device_handle, _command_pool, nullptr);
        vkDestroyDevice(_device_handle, nullptr);
    }

} // namespace render::vulkan
