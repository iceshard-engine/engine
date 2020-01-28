#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>

#include "pipeline_layout.hxx"

namespace iceshard::renderer::vulkan
{

    void create_pipeline_layout(VkDevice device, VulkanResourceLayouts& resource_layouts) noexcept
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VkDescriptorSetLayout> layouts_native{ temp_alloc };
        core::pod::array::push_back(layouts_native, resource_layouts.descriptor_set_uniforms);
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

        auto api_result = vkCreatePipelineLayout(device, &pipeline_create_info, nullptr, &resource_layouts.pipeline_layout);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");
    }

    void destroy_pipeline_layout(VkDevice device, VulkanResourceLayouts const& layouts) noexcept
    {
        vkDestroyPipelineLayout(device, layouts.pipeline_layout, nullptr);
    }

} // namespace iceshard::renderer::vulkan
