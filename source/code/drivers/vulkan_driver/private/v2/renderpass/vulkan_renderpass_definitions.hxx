#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    auto renderpass_forward(VkDevice device, VkFormat format) noexcept -> VkRenderPass;

    auto renderpass_forward_postprocess(VkDevice device, VkFormat format) noexcept -> VkRenderPass;

} // namespace iceshard::renderer::vulkan
