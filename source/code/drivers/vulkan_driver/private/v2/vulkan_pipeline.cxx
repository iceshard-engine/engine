#include <iceshard/renderer/vulkan/vulkan_pipeline.hxx>

#include "pipelines/vulkan_debugui_pipeline.hxx"
#include "pipelines/vulkan_default_pipeline.hxx"
#include "pipelines/vulkan_textured_pipeline.hxx"
#include "pipelines/vulkan_postprocess_pipeline.hxx"

namespace iceshard::renderer::vulkan
{

    void create_graphics_pipeline(
        VulkanDevices devices,
        VulkanRenderPass renderpass,
        VulkanPipelineLayout layout,
        VulkanPipelineModules modules,
        VulkanPipeline& pipeline
    ) noexcept
    {
        pipeline.layout = layout.layout;
        if (layout.layout_type == RenderPipelineLayout::DebugUI)
        {
            create_debugui_pipeline(
                devices.graphics.handle,
                renderpass.renderpass,
                layout.layout,
                modules,
                pipeline.pipeline
            );
        }
        else if (layout.layout_type == RenderPipelineLayout::Default)
        {
            create_default_pipeline(
                devices.graphics.handle,
                renderpass.renderpass,
                layout.layout,
                modules,
                pipeline.pipeline
            );
        }
        else if (layout.layout_type == RenderPipelineLayout::Textured)
        {
            create_textured_pipeline(
                devices.graphics.handle,
                renderpass.renderpass,
                layout.layout,
                modules,
                pipeline.pipeline
            );
        }
        else if (layout.layout_type == RenderPipelineLayout::PostProcess)
        {
            create_postprocess_pipeline(
                devices.graphics.handle,
                renderpass.renderpass,
                layout.layout,
                modules,
                pipeline.pipeline
            );
        }
        else
        {
            IS_ASSERT(false, "Unsupported pipeline type!");
        }
    }

    void destroy_graphics_pipeline(
        VulkanDevices devices,
        VulkanPipeline pipeline
    ) noexcept
    {
        vkDestroyPipeline(devices.graphics.handle, pipeline.pipeline, nullptr);
    }

} // namespace iceshard::renderer::vulkan
