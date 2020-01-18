#pragma once
#include <core/allocator.hxx>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanDescriptorPool
    {
    public:
        VulkanDescriptorPool(VkDevice graphics_device) noexcept;
        ~VulkanDescriptorPool() noexcept;

        auto graphics_device() const noexcept { return _graphics_device; }

        auto native_handle() const noexcept { return _descriptor_pool; }

    private:
        VkDevice const _graphics_device;
        VkDescriptorPool _descriptor_pool;
    };

} // namespace render::vulkan
