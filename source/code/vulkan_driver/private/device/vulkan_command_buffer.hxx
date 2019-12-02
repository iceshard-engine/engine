#pragma once
#include <core/allocator.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanCommandBuffer
    {
    public:
        VulkanCommandBuffer(core::allocator& alloc, VkCommandBuffer handle) noexcept;
        ~VulkanCommandBuffer() noexcept;

        auto native_handle() const noexcept -> VkCommandBuffer { return _command_buffer_handle; }

    private:
        core::allocator& _allocator;
        VkCommandBuffer _command_buffer_handle;
    };

} // namespace render::vulkan
