#pragma once
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_shader_module.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_pipeline_layouts.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanPipeline
    {
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };

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
