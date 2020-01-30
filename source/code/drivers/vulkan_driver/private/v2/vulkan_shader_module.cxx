#include <iceshard/renderer/vulkan/vulkan_shader_module.hxx>
#include <resource/resource_meta.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    void build_pipeline_shaders(
        VulkanDevices devices,
        core::pod::Array<asset::AssetData> const& shader_assets,
        VulkanPipelineModules& pipeline_shaders
    ) noexcept
    {
        IS_ASSERT(
            core::pod::array::size(shader_assets) <= core::size(pipeline_shaders.modules),
            "To many shader assets provided, maximum of {} modules are allowed!",
            core::size(pipeline_shaders.modules)
        );

        VkShaderModuleCreateInfo shader_module_info;
        shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_info.pNext = nullptr;
        shader_module_info.flags = 0;

        uint32_t module_index = 0;
        for (auto const& shader_asset : shader_assets)
        {
            auto const& shader_meta = shader_asset.metadata;
            auto[shader_data, shader_size] = shader_asset.content;

            shader_module_info.codeSize = shader_size;
            shader_module_info.pCode = reinterpret_cast<uint32_t const*>(shader_data);

            {
                int32_t target = resource::get_meta_int32(shader_meta, "shader.target"_sid);
                IS_ASSERT(target == 1, "Only explicit vulkan shaders are supported!");
            }

            {
                int32_t shader_stage = resource::get_meta_int32(shader_meta, "shader.stage"_sid);
                IS_ASSERT(shader_stage == 1 || shader_stage == 2, "Only vertex and fragment shaders are supported!");

                VkShaderStageFlagBits stage_bits = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
                if (shader_stage == 1)
                {
                    stage_bits = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
                }
                else if (shader_stage == 2)
                {
                    stage_bits = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
                }
                pipeline_shaders.stage[module_index] = stage_bits;
            }

            [[maybe_unused]]
            auto api_result = vkCreateShaderModule(
                devices.graphics.handle,
                &shader_module_info,
                nullptr,
                pipeline_shaders.modules + module_index
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create shader module!");

            module_index += 1;
        }
    }

    void release_pipeline_shaders(
        VulkanDevices devices,
        VulkanPipelineModules const& pipeline_shaders
    ) noexcept
    {
        for (auto const& shader_module : pipeline_shaders.modules)
        {
            if (shader_module != nullptr)
            {
                vkDestroyShaderModule(devices.graphics.handle, shader_module, nullptr);
            }
        }
    }

} // namespace iceshard::renderer::vulkan
