#include "vulkan_sampler.hxx"
#include <core/debug/assert.hxx>

namespace render::vulkan
{

    VulkanSampler::VulkanSampler(VkDevice graphics_device, VkSampler sampler) noexcept
        : _graphics_device{ graphics_device }
        , _sampler{ sampler }
    {
    }

    VulkanSampler::~VulkanSampler() noexcept
    {
        vkDestroySampler(_graphics_device, _sampler, nullptr);
    }

    auto create_sampler(core::allocator& alloc, VkDevice graphics_device) noexcept -> core::memory::unique_pointer<VulkanSampler>
    {
        VkSamplerCreateInfo sampler_info;
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.pNext = nullptr;
        sampler_info.flags = 0;
        sampler_info.magFilter = VK_FILTER_LINEAR;
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

        VkSampler sampler;
        auto api_result = vkCreateSampler(graphics_device, &sampler_info, nullptr, &sampler);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create sampler object!");

        return core::memory::make_unique<VulkanSampler>(alloc, graphics_device, sampler);
    }

} // namespace render::vulkan
