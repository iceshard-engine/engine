#pragma once
#include <iceshard/renderer/vulkan/vulkan_pipeline.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    void create_tileset_pipeline(
        VkDevice devices,
        VkRenderPass renderpass,
        VkPipelineLayout layout,
        VulkanPipelineModules modules,
        VkPipeline& pipeline
    ) noexcept;

} // namespace iceshard::renderer::vulkan
