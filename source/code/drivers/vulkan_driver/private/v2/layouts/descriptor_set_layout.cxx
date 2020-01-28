#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>

#include "descriptor_set_layout.hxx"

namespace iceshard::renderer::vulkan
{

    template<>
    void create_descriptor_set_layout<VulkanDescriptorSetGroup::UniformBuffers>(
        VkDevice device,
        VulkanResourceLayouts& resource_layouts
    ) noexcept
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ temp_alloc };

        constexpr auto max_bindings = decltype(temp_alloc)::InternalBufferSize / sizeof(VkDescriptorSetLayoutBinding);
        core::pod::array::reserve(bindings, max_bindings);

        core::pod::array::push_back(bindings,
            VkDescriptorSetLayoutBinding
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            }
        );

        VkDescriptorSetLayoutCreateInfo layout_info;
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = nullptr;
        layout_info.flags = 0;
        layout_info.bindingCount = core::pod::array::size(bindings);
        layout_info.pBindings = core::pod::begin(bindings);

        auto api_result = vkCreateDescriptorSetLayout(
            device,
            &layout_info,
            nullptr,
            &resource_layouts.descriptor_set_uniforms
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor set layout.");
    }


    template<>
    void create_descriptor_set_layout<VulkanDescriptorSetGroup::Samplers>(
        VkDevice device,
        VulkanResourceLayouts& resource_layouts
    ) noexcept
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ temp_alloc };

        auto const max_bindings = core::size(resource_layouts.immutable_samplers);
        core::pod::array::reserve(bindings, max_bindings);

        for (uint32_t binding = 0; binding < max_bindings; ++binding)
        {
            core::pod::array::push_back(bindings,
                VkDescriptorSetLayoutBinding
                {
                    .binding = binding,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = resource_layouts.immutable_samplers + binding
                }
            );
        }

        VkDescriptorSetLayoutCreateInfo layout_info;
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = nullptr;
        layout_info.flags = 0;
        layout_info.bindingCount = core::pod::array::size(bindings);
        layout_info.pBindings = core::pod::begin(bindings);

        auto api_result = vkCreateDescriptorSetLayout(
            device,
            &layout_info,
            nullptr,
            &resource_layouts.descriptor_set_samplers
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor set layout.");
    }


    template<>
    void create_descriptor_set_layout<VulkanDescriptorSetGroup::Textures>(
        VkDevice device,
        VulkanResourceLayouts& resource_layouts
        ) noexcept
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ temp_alloc };

        constexpr auto max_bindings = decltype(temp_alloc)::InternalBufferSize / sizeof(VkDescriptorSetLayoutBinding);
        core::pod::array::reserve(bindings, max_bindings);

        for (uint32_t binding = 0; binding < max_bindings; ++binding)
        {
            core::pod::array::push_back(bindings,
                VkDescriptorSetLayoutBinding
                {
                    .binding = binding,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
            );
        }

        VkDescriptorSetLayoutCreateInfo layout_info;
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = nullptr;
        layout_info.flags = 0;
        layout_info.bindingCount = core::pod::array::size(bindings);
        layout_info.pBindings = core::pod::begin(bindings);

        auto api_result = vkCreateDescriptorSetLayout(
            device,
            &layout_info,
            nullptr,
            &resource_layouts.descriptor_set_textures
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor set layout.");
    }


    void destroy_descriptor_set_layout(
        VkDevice device,
        VkDescriptorSetLayout const& descriptor_set_layout
    ) noexcept
    {
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    }

} // namespace iceshard::renderer::vulkan
