#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanResourcePool
    {
        VkDescriptorPool descriptor_pool;
    };

    struct VulkanImmutableSamplers
    {
        VkSampler samplers[1];
    };

    struct VulkanResourceLayouts
    {
        VkDescriptorSetLayout descriptor_set_uniforms;
        VkDescriptorSetLayout descriptor_set_textures;
        VkDescriptorSetLayout descriptor_set_samplers;
        VkSampler immutable_samplers[1];
    };

    void create_resource_layouts(VkDevice device, VulkanResourceLayouts& resource_layouts) noexcept;

    void destroy_resource_layouts(VkDevice device, VulkanResourceLayouts resource_layouts) noexcept;

    void create_resource_pool(VkDevice, VulkanResourcePool& resource_pool) noexcept;

    void destroy_resource_pool(VkDevice, VulkanResourcePool resource_pool) noexcept;

} // namespace iceshard::renderer::vulkan
