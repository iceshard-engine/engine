#pragma once
#include <core/pointer.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanSampler
    {
    public:
        VulkanSampler(VkDevice graphics_device, VkSampler sampler) noexcept;
        ~VulkanSampler() noexcept;

        auto native_handle() noexcept { return _sampler; }

    private:
        VkDevice _graphics_device;
        VkSampler _sampler;
    };

    auto create_sampler(core::allocator& alloc, VkDevice graphics_device) noexcept -> core::memory::unique_pointer<VulkanSampler>;

} // namespace render::vulkan
