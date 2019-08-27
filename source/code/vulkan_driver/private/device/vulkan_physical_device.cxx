#include "vulkan_physical_device.hxx"
#include <core/pod/array.hxx>

namespace render::vulkan
{

    VulkanPhysicalDevice::VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle) noexcept
        : _allocator{ alloc }
        , _vulkan_physical_device{ device_handle }
    {
        initialize();
    }

    VulkanPhysicalDevice::~VulkanPhysicalDevice() noexcept
    {
        shutdown();
    }

    void VulkanPhysicalDevice::initialize() noexcept
    {
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(_vulkan_physical_device, &queue_family_count, nullptr);

        core::pod::Array<VkQueueFamilyProperties> queue_family_properties{ _allocator };
        core::pod::array::resize(queue_family_properties, queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(_vulkan_physical_device, &queue_family_count, &queue_family_properties[0]);
        fmt::print("Queue families found: {}\n", queue_family_count);
    }

    void VulkanPhysicalDevice::shutdown() noexcept
    {
    }

} // namespace render::vulkan

