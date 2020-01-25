#pragma once
#include <core/allocator.hxx>

#include <iceshard/renderer/vulkan/vulkan_system.hxx>

#include "vulkan_image.hxx"

namespace render::vulkan
{

    class VulkanFramebuffer
    {
    public:
        VulkanFramebuffer(VkDevice device, VkFramebuffer framebuffer_handle) noexcept;
        ~VulkanFramebuffer() noexcept;

        auto native_handle() const noexcept -> VkFramebuffer { return _native_handle; }

    private:
        VkDevice _device_handle;
        VkFramebuffer _native_handle;
    };

    void create_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer*>& results,
        iceshard::renderer::vulkan::VulkanRenderSystem* new_render_system,
        VulkanImage const& depth_buffer,
        VkExtent2D extent
    ) noexcept;

} // namespace render::vulkan
