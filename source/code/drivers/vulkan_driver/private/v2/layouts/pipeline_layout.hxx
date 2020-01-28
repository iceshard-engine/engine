#pragma once
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>

namespace iceshard::renderer::vulkan
{

    void create_pipeline_layout(VkDevice device, VulkanResourceLayouts& resource_layouts) noexcept;

    void destroy_pipeline_layout(VkDevice device, VulkanResourceLayouts const& resource_layouts) noexcept;

} // namespace iceshard::renderer::vulkan
