#pragma once
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_shader_module.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanPipelineLayout
    {
        VkPipelineLayout layout;
        RenderPipelineLayout layout_type;
    };

    struct VulkanPipelineLayouts
    {
        VulkanPipelineLayout default_layout;
        VulkanPipelineLayout debugui_layout;
    };

    struct VulkanPipeline
    {
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };

    void create_pipeline_layouts(
        VulkanDevices devices,
        VulkanResourceLayouts const& resource_layouts,
        VulkanPipelineLayouts& pipeline_layouts
    ) noexcept;

    void destroy_pipeline_layouts(
        VulkanDevices devices,
        VulkanPipelineLayouts layouts
    ) noexcept;

    void query_vertex_input_descriptions(
        RenderPipelineLayout pipeline_layout,
        core::pod::Array<VkVertexInputBindingDescription>& bindings,
        core::pod::Array<VkVertexInputAttributeDescription>& attributes
    ) noexcept;

    void create_graphics_pipeline(
        VulkanDevices devices,
        VulkanRenderPass renderpass,
        VulkanPipelineLayout layout,
        VulkanPipelineModules modules,
        VulkanPipeline& pipeline
    ) noexcept;

    void destroy_graphics_pipeline(
        VulkanDevices devices,
        VulkanPipeline pipeline
    ) noexcept;

} // namespace iceshard::renderer::vulkan
