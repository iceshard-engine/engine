#include "vulkan_command_buffer.hxx"

namespace render::vulkan
{

    VulkanCommandBuffer::VulkanCommandBuffer(core::allocator& alloc, VkCommandBuffer handle) noexcept
        : _allocator{ alloc }
        , _command_buffer_handle{ handle }
    {
    }

    VulkanCommandBuffer::~VulkanCommandBuffer() noexcept
    {
    }

} // namespace render::vulkan

