#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/hash.hxx>

#include "vulkan_device.hxx"

namespace render::vulkan
{

    //! \brief A single vulkan device.
    class VulkanPhysicalDevice final
    {
    public:
        VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle, VkSurfaceKHR surface_handle) noexcept;
        ~VulkanPhysicalDevice() noexcept;

        auto native_handle() const noexcept -> VkPhysicalDevice { return _physical_device_handle; }

        auto surface_handle() const noexcept -> VkSurfaceKHR { return _surface_handle; }

        auto surface_capabilities() const noexcept -> VkSurfaceCapabilitiesKHR const& { return _surface_capabilities; }

        auto surface_present_modes() const noexcept -> core::pod::Array<VkPresentModeKHR> const& { return _present_modes; }

        auto surface_surface_formats() const noexcept -> core::pod::Array<VkSurfaceFormatKHR> const& { return _surface_formats; }

        auto graphics_device() noexcept -> VulkanDevice* { return _graphics_device.get(); }

        auto graphics_device() const noexcept -> VulkanDevice const* { return _graphics_device.get(); }

    protected:
        void initialize() noexcept;

        void enumerate_family_queues() noexcept;
        void enumerate_surface_capabilities() noexcept;
        void enumerate_surface_present_modes() noexcept;
        void enumerate_surface_formats() noexcept;

        void create_device(VulkanDeviceQueueType queue_type) noexcept;

        void shutdown() noexcept;

    private:
        core::allocator& _allocator;

        // A valid vulkan physical device handle.
        VkPhysicalDevice _physical_device_handle;

        VkSurfaceKHR _surface_handle;

        // A structure holding data about a single device factory.
        struct VulkanDeviceFactory
        {
            using FactroyFunctionSignature = auto(core::allocator&, VkPhysicalDevice, VulkanQueueFamilyIndex) noexcept -> VkDevice;

            FactroyFunctionSignature* factory_function;

            VkQueueFamilyProperties family_properties;

            VulkanQueueFamilyIndex family_index;

            bool supports_present;
        };

        // Physical device surface data.
        VkSurfaceCapabilitiesKHR _surface_capabilities{};

        core::pod::Array<VkPresentModeKHR> _present_modes;

        core::pod::Array<VkSurfaceFormatKHR> _surface_formats;

        // An array of available logical device factories.
        core::pod::Hash<VulkanDeviceFactory> _device_factories;
        core::pod::Hash<VkCommandPool> _command_pools;

        core::memory::unique_pointer<VulkanDevice> _graphics_device;
    };

} // namespace render::vulkan