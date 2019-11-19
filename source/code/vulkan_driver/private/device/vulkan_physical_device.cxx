#include "vulkan_physical_device.hxx"
#include <core/pod/array.hxx>

namespace render::vulkan
{
    namespace detail
    {

        // Array of all supported queue flags.
        VkQueueFlagBits queue_flags_array[] = {
            VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT, VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT, VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT, VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT
        };

        // Graphics queue vulkan device factory
        auto graphics_queue_vulkan_device_factory(
            [[maybe_unused]] core::allocator& alloc,
            VkPhysicalDevice physical_device,
            VulkanQueueFamilyIndex family_index) noexcept -> VkDevice
        {
            VkDeviceQueueCreateInfo queue_info{};

            float queue_priorities[1] = { 0.0 };
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.pNext = nullptr;
            queue_info.queueCount = 1;
            queue_info.pQueuePriorities = queue_priorities;
            queue_info.queueFamilyIndex = static_cast<uint32_t>(family_index);

            VkDeviceCreateInfo device_info = {};
            device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_info.pNext = nullptr;
            device_info.queueCreateInfoCount = 1;
            device_info.pQueueCreateInfos = &queue_info;
            device_info.enabledExtensionCount = 0;
            device_info.ppEnabledExtensionNames = nullptr;
            device_info.enabledLayerCount = 0;
            device_info.ppEnabledLayerNames = nullptr;
            device_info.pEnabledFeatures = nullptr;

            VkDevice device;
            VkResult res = vkCreateDevice(physical_device, &device_info, nullptr, &device);
            IS_ASSERT(res == VkResult::VK_SUCCESS, "Error during creation of logical device!");

            return device;
        }

        // Compute queue vulkan device factory
        auto compute_queue_vulkan_device_factory(
            [[maybe_unused]] core::allocator& alloc,
            [[maybe_unused]] VkPhysicalDevice physical_device,
            [[maybe_unused]] VulkanQueueFamilyIndex family_index) noexcept -> VkDevice
        {
            return nullptr;
        }

        // Transfer queue vulkan device factory
        auto transfer_queue_vulkan_device_factory(
            [[maybe_unused]] core::allocator& alloc,
            [[maybe_unused]] VkPhysicalDevice physical_device,
            [[maybe_unused]] VulkanQueueFamilyIndex family_index) noexcept -> VkDevice
        {
            return nullptr;
        }

        // Sparse queue vulkan device factory
        auto sparse_queue_vulkan_device_factory(
            [[maybe_unused]] core::allocator& alloc,
            [[maybe_unused]] VkPhysicalDevice physical_device,
            [[maybe_unused]] VulkanQueueFamilyIndex family_index) noexcept -> VkDevice
        {
            return nullptr;
        }

        auto queue_type_for_flag_bit(VkQueueFlagBits flag) noexcept
        {
            switch (flag)
            {
            case VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT:
                return VulkanDeviceQueueType::GraphicsQueue;
            case VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT:
                return VulkanDeviceQueueType::ComputeQueue;
            case VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT:
                return VulkanDeviceQueueType::TransferQueue;
            case VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT:
                return VulkanDeviceQueueType::SparseQueue;
            };

            IS_ASSERT(false, "Failed to get queue type from flag! THIS SHOULD NOT HAPPEN!");
            std::abort();
            return static_cast<VulkanDeviceQueueType>(0);
        }

        auto factory_for_flag_bit(VkQueueFlagBits flag) noexcept
        {
            using result_type = auto (*)(core::allocator&, VkPhysicalDevice, VulkanQueueFamilyIndex) noexcept->VkDevice;
            result_type result = nullptr;

            switch (flag)
            {
            case VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT:
                result = &graphics_queue_vulkan_device_factory;
                break;
            case VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT:
                result = &compute_queue_vulkan_device_factory;
                break;
            case VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT:
                result = &transfer_queue_vulkan_device_factory;
                break;
            case VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT:
                result = &sparse_queue_vulkan_device_factory;
                break;
            };
            return result;
        }

    } // namespace detail

    VulkanPhysicalDevice::VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle) noexcept
        : _allocator{ alloc }
        , _vulkan_physical_device{ device_handle }
        , _device_factories{ _allocator }
        , _command_pools{ _allocator }
        , _logical_devices{ _allocator }
    {
        initialize();
    }

    VulkanPhysicalDevice::~VulkanPhysicalDevice() noexcept
    {
        shutdown();
    }

    auto VulkanPhysicalDevice::create_device(VulkanDeviceQueueType queue_type) noexcept -> VulkanDevice*
    {
        VulkanDevice* result = nullptr;

        auto queue_typeid = static_cast<uint64_t>(queue_type);
        auto* it = core::pod::multi_hash::find_first(_device_factories, queue_typeid);
        if (it != nullptr)
        {
            auto& factory_data = it->value;
            VkDevice device_handle = factory_data.factory_function(_allocator, _vulkan_physical_device, factory_data.family_index);

            result = _allocator.make<VulkanDevice>(_allocator, queue_type, factory_data.family_index, device_handle);
            core::pod::array::push_back(_logical_devices, result);
        }

        return result;
    }

    void VulkanPhysicalDevice::initialize() noexcept
    {
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(_vulkan_physical_device, &queue_family_count, nullptr);

        core::pod::Array<VkQueueFamilyProperties> queue_family_properties{ _allocator };
        core::pod::array::resize(queue_family_properties, queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(_vulkan_physical_device, &queue_family_count, &queue_family_properties[0]);

        // Going through all queue family properties.
        for (uint32_t index = 0; index < queue_family_count; ++index)
        {
            const auto& queue_family_property = queue_family_properties[index];

            // Check each familiy flags
            for (auto flag_bit : detail::queue_flags_array)
            {
                if (static_cast<VkQueueFlagBits>(queue_family_property.queueFlags & flag_bit) == flag_bit)
                {
                    if (auto* queue_factory = detail::factory_for_flag_bit(flag_bit))
                    {
                        VulkanDeviceFactory factory{
                            .factory_function = queue_factory,
                            .family_properties = queue_family_property,
                            .family_index = VulkanQueueFamilyIndex{ index }
                        };

                        auto queue_type = static_cast<uint64_t>(detail::queue_type_for_flag_bit(flag_bit));
                        core::pod::multi_hash::insert(_device_factories, queue_type, factory);
                    }
                }
            }
        }
    }

    void VulkanPhysicalDevice::shutdown() noexcept
    {
        for (auto* device : _logical_devices)
        {
            _allocator.destroy(device);
        }
    }

} // namespace render::vulkan
