#include "vulkan_device_factories.hxx"

namespace iceshard::renderer::vulkan
{

    void create_graphics_device(
        core::pod::Array<VulkanDeviceQueueFamily> const& queue_families,
        VkPhysicalDevice physical_device,
        VulkanGraphicsDevice& device
    ) noexcept
    {
        constexpr uint32_t invalid_queue_index = std::numeric_limits<uint32_t>::max();
        // #todo This implementation uses a quick assumption that graphics and presenting are supported on the same queue

        device.family_index = invalid_queue_index;
        for (auto const& family : queue_families)
        {
            if (family.supports_graphics && family.supports_presenting)
            {
                device.family_index = family.queue_index;
                break;
            }
        }

        IS_ASSERT(device.family_index != invalid_queue_index, "No queue family was found which supports graphics and presenting!");

        // Prepare device creation data
        const char* extension_names[] =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        float queue_priorities[1] =
        {
            0.0
        };

        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = queue_priorities;
        queue_info.queueFamilyIndex = device.family_index;

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = core::size(extension_names);
        device_info.ppEnabledExtensionNames = &extension_names[0];
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

        VkResult api_result = vkCreateDevice(physical_device, &device_info, nullptr, &device.handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Error during creation of logical device!");

        vkGetDeviceQueue(device.handle, device.family_index, 0, &device.queue);
    }

    void release_graphics_device(
        VulkanGraphicsDevice device
    ) noexcept
    {
        vkDestroyDevice(device.handle, nullptr);
    }

} // namespace iceshard::renderer::vulkan
