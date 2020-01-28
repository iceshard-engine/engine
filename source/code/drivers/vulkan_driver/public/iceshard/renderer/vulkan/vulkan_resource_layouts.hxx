#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanImmutableSamplers
    {
        VkSampler samplers[1];
    };

    struct VulkanResourceLayouts
    {
        VkPipelineLayout pipeline_layout;
        VkDescriptorSetLayout descriptor_set_uniforms;
        VkDescriptorSetLayout descriptor_set_textures;
        VkDescriptorSetLayout descriptor_set_samplers;
        VkSampler immutable_samplers[1];
    };

    void create_resource_layouts(VkDevice device, VulkanResourceLayouts& resource_layouts) noexcept;

    void destroy_resource_layouts(VkDevice device, VulkanResourceLayouts resource_layouts) noexcept;

} // namespace iceshard::renderer::vulkan
