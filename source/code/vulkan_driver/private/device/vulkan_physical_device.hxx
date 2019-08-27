#pragma once
#include <core/allocator.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{


    //! \brief A single vulkan device.
    class VulkanPhysicalDevice final
    {
    public:
        VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle) noexcept;
        ~VulkanPhysicalDevice() noexcept;

        void initialize() noexcept;

        void shutdown() noexcept;

    private:
        core::allocator& _allocator;

        // A valid vulkan physical device handle.
        VkPhysicalDevice _vulkan_physical_device;

        // A logical vulkan device.
        VkDevice _vulkan_device{ };
    };


} // namespace render::vulkan