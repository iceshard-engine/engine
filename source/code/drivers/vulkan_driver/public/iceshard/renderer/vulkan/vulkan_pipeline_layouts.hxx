#pragma once
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
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
        VulkanPipelineLayout tiled_layout;
        VulkanPipelineLayout textured_layout;
        VulkanPipelineLayout postprocess_layout;
        VulkanPipelineLayout debugui_layout;
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

} // namespace iceshard::renderer::vulkan
