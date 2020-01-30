#pragma once
#include <asset_system/asset.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_shader_module.hxx>

namespace iceshard::renderer::vulkan
{

    enum class ShaderStages
    {
        Vertex,
        Fragment,
    };

    struct VulkanPipelineModules
    {
        VkShaderModule modules[2];
        VkShaderStageFlagBits stage[2];
    };

    void build_pipeline_shaders(
        VulkanDevices devices,
        core::pod::Array<asset::AssetData> const& shader_assets,
        VulkanPipelineModules& pipeline_shaders
    ) noexcept;

    void release_pipeline_shaders(
        VulkanDevices devices,
        VulkanPipelineModules const& pipeline_shaders
    ) noexcept;

} // namespace iceshard::renderer::vulkan
