#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>
#include "layouts/descriptor_set_layout.hxx"
#include "layouts/pipeline_layout.hxx"

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        template<uint32_t SamplerCount>
        void create_immutable_samplers(VkDevice device, VkSampler(&samplers)[SamplerCount]) noexcept
        {
            VkSamplerCreateInfo sampler_info;
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.pNext = nullptr;
            sampler_info.flags = 0;
            sampler_info.magFilter = VK_FILTER_LINEAR;

            for (uint32_t idx = 0; idx < SamplerCount; ++idx)
            {
                sampler_info.minFilter = VK_FILTER_LINEAR;
                sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_info.anisotropyEnable = VK_FALSE;
                sampler_info.maxAnisotropy = 1;
                sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                sampler_info.unnormalizedCoordinates = VK_FALSE;
                sampler_info.compareEnable = VK_FALSE;
                sampler_info.compareOp = VK_COMPARE_OP_LESS;
                sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler_info.mipLodBias = 0.0f;
                sampler_info.minLod = 0.0f;
                sampler_info.maxLod = 0.0f;

                auto api_result = vkCreateSampler(device, &sampler_info, nullptr, samplers + idx);
                IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create sampler object!");
            }
        }

        template<uint32_t SamplerCount>
        void destroy_immutable_samplers(VkDevice device, VkSampler(&samplers)[SamplerCount]) noexcept
        {
            for (auto sampler : samplers)
            {
                vkDestroySampler(device, sampler, nullptr);
            }
        }

    } // namespace detail

    void create_resource_layouts(VkDevice device, VulkanResourceLayouts& resource_layouts) noexcept
    {
        detail::create_immutable_samplers(device, resource_layouts.immutable_samplers);

        create_descriptor_set_layout<VulkanDescriptorSetGroup::UniformBuffers>(
            device,
            resource_layouts
        );
        create_descriptor_set_layout<VulkanDescriptorSetGroup::Samplers>(
            device,
            resource_layouts
        );
        create_descriptor_set_layout<VulkanDescriptorSetGroup::Textures>(
            device,
            resource_layouts
        );

        create_pipeline_layout(device, resource_layouts);
    }

    void destroy_resource_layouts(VkDevice device, VulkanResourceLayouts resource_layouts) noexcept
    {
        destroy_pipeline_layout(device, resource_layouts);

        destroy_descriptor_set_layout(device, resource_layouts.descriptor_set_uniforms);
        destroy_descriptor_set_layout(device, resource_layouts.descriptor_set_samplers);
        destroy_descriptor_set_layout(device, resource_layouts.descriptor_set_textures);

        detail::destroy_immutable_samplers(device, resource_layouts.immutable_samplers);
    }

    void create_resource_pool(VkDevice device, VulkanResourcePool& resource_pool) noexcept
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info;
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.pNext = nullptr;
        pool_info.flags = 0;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = core::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        vkCreateDescriptorPool(device, &pool_info, nullptr, &resource_pool.descriptor_pool);
    }

    void destroy_resource_pool(VkDevice device, VulkanResourcePool resource_pool) noexcept
    {
        vkDestroyDescriptorPool(device, resource_pool.descriptor_pool, nullptr);
    }

} // namespace iceshard::renderer::vulkan
