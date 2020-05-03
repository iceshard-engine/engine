#pragma once
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/vulkan/vulkan_pipeline.hxx>
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>
#include <iceshard/renderer/vulkan/vulkan_framebuffer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanResourceSet
    {
        core::stringid_type name;

        VkPipelineLayout pipeline_layout;
        VkDescriptorSet descriptor_sets[3];
    };

    void create_resource_set(
        VkDevice device,
        VulkanFramebuffer framebuffer,
        VulkanResourcePool resource_pool,
        VulkanPipelineLayout pipeline_layout,
        VulkanResourceLayouts const& resource_layouts,
        core::stringid_arg_type name,
        core::pod::Array<RenderResource> const& resources,
        VulkanResourceSet& set
    ) noexcept;

    void update_resource_set(
        VkDevice device,
        VulkanFramebuffer framebuffer,
        VulkanResourceSet& set,
        core::pod::Array<RenderResource> const& resources
    ) noexcept;

    void destroy_resource_set(
        VkDevice device,
        VulkanResourcePool resource_pool,
        VulkanResourceSet const& set
    ) noexcept;

} // namespace iceshard::renderer::vulkan
