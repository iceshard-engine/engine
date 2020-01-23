#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    class VulkanRenderPass;

    template<RenderPassType>
    auto create_renderpass(VkDevice device, VkFormat attachment_format) noexcept -> VkRenderPass;

} // namespace iceshard::renderer::vulkan
