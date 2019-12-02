#include "vulkan_pipeline_layout.hxx"

namespace render::vulkan
{

    VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, VkPipelineLayout pipeline_layout) noexcept
        : _device_handle{ device }
        , _native_handle{ pipeline_layout }
    {
    }

    VulkanPipelineLayout::~VulkanPipelineLayout() noexcept
    {
        vkDestroyPipelineLayout(_device_handle, _native_handle, nullptr);
    }

    auto create_pipeline_layout(core::allocator& alloc, VkDevice device, core::pod::Array<VkDescriptorSetLayout> const& descriptor_sets) noexcept -> core::memory::unique_pointer<VulkanPipelineLayout>
    {
        VkPipelineLayoutCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.pushConstantRangeCount = 0;
        pipeline_create_info.pPushConstantRanges = NULL;
        pipeline_create_info.setLayoutCount = core::pod::array::size(descriptor_sets);
        pipeline_create_info.pSetLayouts = core::pod::array::begin(descriptor_sets);

        VkPipelineLayout pipeline_layout;
        auto api_result = vkCreatePipelineLayout(device, &pipeline_create_info, nullptr, &pipeline_layout);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");

        return core::memory::make_unique<VulkanPipelineLayout>(alloc, device, pipeline_layout);
    }

} // namespace render::vulkan
