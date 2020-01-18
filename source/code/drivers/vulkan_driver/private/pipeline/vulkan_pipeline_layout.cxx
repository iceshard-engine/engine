#include "vulkan_pipeline_layout.hxx"
#include "vulkan_descriptor_set_layout.hxx"
#include <core/collections.hxx>

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

    auto create_pipeline_layout(
        core::allocator& alloc,
        VkDevice device,
        core::Vector<core::memory::unique_pointer<VulkanDescriptorSetLayout>> const& layouts
    ) noexcept -> core::memory::unique_pointer<VulkanPipelineLayout>
    {
        core::pod::Array<VkDescriptorSetLayout> layouts_native{ alloc };
        core::pod::array::reserve(layouts_native, static_cast<uint32_t>(layouts.size()));
        for (auto const& layout : layouts)
        {
            core::pod::array::push_back(layouts_native, layout->native_handle());
        }

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

        VkPipelineLayout pipeline_layout;
        auto api_result = vkCreatePipelineLayout(device, &pipeline_create_info, nullptr, &pipeline_layout);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline layout!");

        return core::memory::make_unique<VulkanPipelineLayout>(alloc, device, pipeline_layout);
    }

} // namespace render::vulkan
