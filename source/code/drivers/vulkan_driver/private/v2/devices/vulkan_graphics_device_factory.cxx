#include "vulkan_device_factories.hxx"

namespace iceshard::renderer::vulkan
{

    void create_graphics_device(
        core::pod::Array<VulkanDeviceQueueFamily> const& queue_families,
        [[maybe_unused]] VulkanGraphicsDevice& device
    ) noexcept
    {
        constexpr uint32_t invalid_queue_index = std::numeric_limits<uint32_t>::max();
        // #todo This implementation uses a quick assumption that graphics and presenting are supported on the same queue

        uint32_t first_supported_queue = invalid_queue_index;
        for (auto const& family : queue_families)
        {
            if (family.supports_graphics && family.supports_presenting)
            {
                first_supported_queue = family.queue_index;
                break;
            }
        }

        IS_ASSERT(first_supported_queue != invalid_queue_index, "No queue family was found which supports graphics and presenting!");
    }

} // namespace iceshard::renderer::vulkan
