#include "vulkan_device_factories.hxx"
#include <core/allocators/stack_allocator.hxx>

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        struct QueueFlagEntry
        {
            VkQueueFlags flag;
            bool VulkanDeviceQueueFamily::* field;
        };

        // Array of all supported queue flags.
        constexpr QueueFlagEntry queue_flags_references[] = {
            QueueFlagEntry{
                .flag = VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT,
                .field = &VulkanDeviceQueueFamily::supports_graphics
            },
            QueueFlagEntry{
                .flag = VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT,
                .field = &VulkanDeviceQueueFamily::supports_compute
            },
            QueueFlagEntry{
                .flag = VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT,
                .field = &VulkanDeviceQueueFamily::supports_transfer
            },
            //QueueFlagEntry{
            //    .flag = VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT,
            //    .field = &VulkanDeviceQueueFamily::supports_presenting
            //}
        };

        auto create_device_queue(VkQueueFamilyProperties const& queue_props) noexcept -> VulkanDeviceQueueFamily
        {
            VulkanDeviceQueueFamily result;

            // Save number of queues we can create
            result.queue_count = queue_props.queueCount;

            // Check family capabilities
            for (auto entry : queue_flags_references)
            {
                (result.*entry.field) = (queue_props.queueFlags & entry.flag) == entry.flag;
            }

            return result;
        }

    } // namespace detail

    void query_physical_device_queue_families(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        core::pod::Array<VulkanDeviceQueueFamily>& queues
    ) noexcept
    {
        core::memory::stack_allocator_1024 temp_alloc;

        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

        core::pod::Array<VkQueueFamilyProperties> queue_family_properties{ temp_alloc };
        core::pod::array::resize(queue_family_properties, queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, &queue_family_properties[0]);

        // Iterate over all queue families.
        for (uint32_t index = 0; index < queue_family_count; ++index)
        {
            VulkanDeviceQueueFamily device_queue = detail::create_device_queue(queue_family_properties[index]);
            device_queue.queue_index = index;

            // Check if family supports presenting
            VkBool32 supports_present = VK_FALSE;
            auto api_result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &supports_present);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query information if family {} (index) supports presenting!", index);
            device_queue.supports_presenting = supports_present == VK_TRUE;

            // Save the presenting result and push the result
            core::pod::array::push_back(queues, std::move(device_queue));
        }
    }

} // namespace iceshard::renderer::vulkan
