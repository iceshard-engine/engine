#include "vulkan_descriptor_pool.hxx"
#include <core/debug/assert.hxx>

namespace render::vulkan
{

    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice graphics_device) noexcept
        : _graphics_device{ graphics_device }
        , _descriptor_pool{ nullptr }
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

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;

        auto api_result = vkCreateDescriptorPool(_graphics_device, &pool_info, nullptr, &_descriptor_pool);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Descriptor pool creation failed!");
    }

    VulkanDescriptorPool::~VulkanDescriptorPool() noexcept
    {
        vkDestroyDescriptorPool(_graphics_device, _descriptor_pool, nullptr);
    }

} // namespace render::vulkan
