#pragma once
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanDescriptorSetGroup
    {
        UniformBuffers,
        Samplers,
        Textures,
    };

    template<VulkanDescriptorSetGroup>
    void create_descriptor_set_layout(
        VkDevice device,
        VulkanResourceLayouts& resource_layouts
    ) noexcept;

    void destroy_descriptor_set_layout(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout
    ) noexcept;

} // namespace iceshard::renderer::vulkan
