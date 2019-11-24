#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanRenderPass
    {
    public:
        VulkanRenderPass(VkDevice device, VkRenderPass render_pass) noexcept;
        ~VulkanRenderPass() noexcept;

    private:
        VkDevice _device_handle;
        VkRenderPass _native_handle;
    };

    auto create_render_pass(core::allocator& alloc, VkDevice device, VkFormat color_format, VkFormat depth_format) noexcept -> core::memory::unique_pointer<VulkanRenderPass>;

} // namespace render::vulkan
