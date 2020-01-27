#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <core/allocators/stack_allocator.hxx>
#include "devices/vulkan_device_factories.hxx"

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto to_string(VkPhysicalDeviceType type) noexcept
        {
            switch (type)
            {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return "integrated-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return "discrete-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return "virtual-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return "cpu";
            default:
                return "unknown";
            }
        }

    } // namespace detail

    bool create_devices(VkInstance instance, VkSurfaceKHR surface, VulkanDevices& devices) noexcept
    {
        uint32_t physical_device_count;

        VkResult res = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query the number of available vulkan devices!");

        if (physical_device_count > 1)
        {
            fmt::print(stderr, "Warning : found more than one device! Picking first returned device by default!\n");
            physical_device_count = 1;
        }

        vkEnumeratePhysicalDevices(instance, &physical_device_count, &devices.physical.handle);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query available vulkan devices!");

        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(devices.physical.handle, &physical_device_properties);

        fmt::print("Physical device properties:\n");
        fmt::print("- vendor.id: {}\n", physical_device_properties.vendorID);
        fmt::print("- device.id: {}\n", physical_device_properties.deviceID);
        fmt::print("- device.type: {}\n", detail::to_string(physical_device_properties.deviceType));
        fmt::print("- device.name: {}\n", physical_device_properties.deviceName);
        fmt::print("- version.driver: {}\n", physical_device_properties.driverVersion);
        fmt::print("- version.api: {}\n", physical_device_properties.apiVersion);

        // Query device families.
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Array<VulkanDeviceQueueFamily> queue_families{ temp_alloc };
        query_physical_device_queue_families(devices.physical.handle, surface, queue_families);

        // Create graphics device
        create_graphics_device(queue_families, devices.physical.handle, devices.graphics);
        return true;
    }

    bool find_memory_type_index(
        VulkanDevices devices,
        VkMemoryRequirements memory_requirements,
        VkMemoryPropertyFlags property_bits,
        uint32_t& type_index_out
    ) noexcept
    {
        VkPhysicalDeviceMemoryProperties device_memory_properties;
        vkGetPhysicalDeviceMemoryProperties(devices.physical.handle, &device_memory_properties);

        for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; i++)
        {
            if ((memory_requirements.memoryTypeBits & 1) == 1)
            {
                // Type is available, does it match user properties?
                if ((device_memory_properties.memoryTypes[i].propertyFlags & property_bits) == property_bits)
                {
                    type_index_out = i;
                    return true;
                }
            }
            memory_requirements.memoryTypeBits >>= 1;
        }
        return false;
    }

    void destroy_devices(VkInstance, VulkanDevices devices) noexcept
    {
        release_graphics_device(devices.graphics);
    }

} // namespace iceshard::renderer::vulkan
