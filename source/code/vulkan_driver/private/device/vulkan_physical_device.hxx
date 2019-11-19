#pragma once
#include <core/allocator.hxx>
#include <core/pod/hash.hxx>

#include "vulkan_device.hxx"

namespace render::vulkan
{

    //! \brief A single vulkan device.
    class VulkanPhysicalDevice final
    {
    public:
        VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle) noexcept;
        ~VulkanPhysicalDevice() noexcept;

        auto create_device(VulkanDeviceQueueType queue_type) noexcept -> VulkanDevice*;

    protected:
        void initialize() noexcept;

        void shutdown() noexcept;

    private:
        core::allocator& _allocator;

        // A valid vulkan physical device handle.
        VkPhysicalDevice _vulkan_physical_device;

        // A structure holding data about a single device factory.
        struct VulkanDeviceFactory
        {
            using FactroyFunctionSignature = auto(core::allocator&, VkPhysicalDevice, VulkanQueueFamilyIndex) noexcept -> VkDevice;

            FactroyFunctionSignature* factory_function;

            VkQueueFamilyProperties family_properties;

            VulkanQueueFamilyIndex family_index;
        };

        // An array of available logical device factories.
        core::pod::Hash<VulkanDeviceFactory> _device_factories;
        core::pod::Hash<VkCommandPool> _command_pools;
        core::pod::Array<render::vulkan::VulkanDevice*> _logical_devices;
    };

} // namespace render::vulkan