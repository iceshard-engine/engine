#include "vulkan_pipeline_layout.hxx"
#include "vulkan_descriptor_set_layout.hxx"

namespace render::vulkan
{

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, core::pod::Array<VkDescriptorSetLayoutBinding> bindings) noexcept
        : _device_handle{ device }
        , _native_handle{ descriptor_set_layout }
        , _layout_bindings{ std::move(bindings) }
    {
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() noexcept
    {
        vkDestroyDescriptorSetLayout(_device_handle, _native_handle, nullptr);
    }

    auto create_descriptor_set_layout(core::allocator& alloc, VkDevice device, core::pod::Array<VkDescriptorSetLayoutBinding> const& layout_bindings) noexcept -> core::memory::unique_pointer<VulkanDescriptorSetLayout>
    {
        //VkDescriptorSetLayoutBinding layout_binding = {};
        //layout_binding.binding = 0;
        //layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        //layout_binding.descriptorCount = 1;
        //layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        //layout_binding.pImmutableSamplers = NULL;
        core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ alloc };
        bindings = layout_bindings;

        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = core::pod::array::size(bindings);
        descriptor_layout.pBindings = core::pod::array::begin(bindings);

        VkDescriptorSetLayout descriptor_set_layout;
        auto api_result = vkCreateDescriptorSetLayout(device, &descriptor_layout, nullptr, &descriptor_set_layout);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor set layout!");

        return core::memory::make_unique<VulkanDescriptorSetLayout>(alloc, device, descriptor_set_layout, std::move(bindings));
    }

} // namespace render::vulkan
