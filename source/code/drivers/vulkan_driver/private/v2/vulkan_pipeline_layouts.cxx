#include <iceshard/renderer/vulkan/vulkan_pipeline_layouts.hxx>

#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    void create_pipeline_layouts(
        VulkanDevices devices,
        VulkanResourceLayouts const& resource_layouts,
        VulkanPipelineLayouts& layouts
    ) noexcept
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VkDescriptorSetLayout> layouts_native{ temp_alloc };
        core::pod::array::push_back(layouts_native, resource_layouts.descriptor_set_uniforms[0]);
        core::pod::array::push_back(layouts_native, resource_layouts.descriptor_set_uniforms[1]);
        core::pod::array::push_back(layouts_native, resource_layouts.descriptor_set_samplers);
        core::pod::array::push_back(layouts_native, resource_layouts.descriptor_set_textures);

        VkPushConstantRange push_constants[1] = {};
        push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 4;

        VkPipelineLayoutCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.pushConstantRangeCount = 1;
        pipeline_create_info.pPushConstantRanges = push_constants;
        pipeline_create_info.setLayoutCount = core::pod::array::size(layouts_native);
        pipeline_create_info.pSetLayouts = core::pod::array::begin(layouts_native);

        {
            layouts.debugui_layout.layout_type = RenderPipelineLayout::DebugUI;
            auto api_result = vkCreatePipelineLayout(
                devices.graphics.handle,
                &pipeline_create_info,
                nullptr,
                &layouts.debugui_layout.layout
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");
        }

        {
            layouts.textured_layout.layout_type = RenderPipelineLayout::Textured;
            auto api_result = vkCreatePipelineLayout(
                devices.graphics.handle,
                &pipeline_create_info,
                nullptr,
                &layouts.textured_layout.layout
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");
        }

        {
            pipeline_create_info.pushConstantRangeCount = 0;
            pipeline_create_info.pPushConstantRanges = nullptr;

            layouts.default_layout.layout_type = RenderPipelineLayout::Default;
            auto api_result = vkCreatePipelineLayout(
                devices.graphics.handle,
                &pipeline_create_info,
                nullptr,
                &layouts.default_layout.layout
            );
            layouts.postprocess_layout.layout_type = RenderPipelineLayout::PostProcess;
            layouts.postprocess_layout.layout = layouts.default_layout.layout;
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");
        }
    }

    void destroy_pipeline_layouts(
        VulkanDevices devices,
        VulkanPipelineLayouts layouts
    ) noexcept
    {
        vkDestroyPipelineLayout(devices.graphics.handle, layouts.debugui_layout.layout, nullptr);
        vkDestroyPipelineLayout(devices.graphics.handle, layouts.default_layout.layout, nullptr);
        vkDestroyPipelineLayout(devices.graphics.handle, layouts.textured_layout.layout, nullptr);
    }

} // namespace iceshard::renderer::vulkan
