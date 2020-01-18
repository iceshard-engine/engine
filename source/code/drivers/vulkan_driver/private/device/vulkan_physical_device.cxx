#include "vulkan_physical_device.hxx"
#include <core/pod/array.hxx>

namespace render::vulkan
{
    namespace detail
    {

        // Array of all supported queue flags.
        VkQueueFlagBits queue_flags_array[] = {
            VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT,
            VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT,
            VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT,
            VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT,
        };

        // Graphics queue vulkan device factory
        auto graphics_queue_vulkan_device_factory(
            [[maybe_unused]] core::allocator& alloc,
            VkPhysicalDevice physical_device,
            VulkanQueueFamilyIndex family_index) noexcept -> VkDevice
        {
            const char* instanceExtensionNames[] = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            };

            VkDeviceQueueCreateInfo queue_info{};

            float queue_priorities[1] = { 0.0 };
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.pNext = nullptr;
            queue_info.queueCount = 1;
            queue_info.pQueuePriorities = queue_priorities;
            queue_info.queueFamilyIndex = static_cast<uint32_t>(family_index);

            //VkPhysicalDeviceFeatures device_features{};
            //device_features.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo device_info = {};
            device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_info.pNext = nullptr;
            device_info.queueCreateInfoCount = 1;
            device_info.pQueueCreateInfos = &queue_info;
            device_info.enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensionNames));
            device_info.ppEnabledExtensionNames = &instanceExtensionNames[0];
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

    VulkanPhysicalDevice::VulkanPhysicalDevice(core::allocator& alloc, VkPhysicalDevice device_handle, VkSurfaceKHR surface_handle) noexcept
        : _allocator{ alloc }
        , _physical_device_handle{ device_handle }
        , _surface_handle{ surface_handle }
        , _present_modes{ _allocator }
        , _surface_formats{ _allocator }
        , _device_factories{ _allocator }
        , _command_pools{ _allocator }
        , _graphics_device{ nullptr, { _allocator } }
    {
        initialize();
    }

    VulkanPhysicalDevice::~VulkanPhysicalDevice() noexcept
    {
        shutdown();
    }

    bool VulkanPhysicalDevice::find_memory_type_index(
        VkMemoryRequirements memory_requirements,
        VkMemoryPropertyFlags property_bits,
        uint32_t& type_index_out) const noexcept
    {
        for (uint32_t i = 0; i < _device_memory_properties.memoryTypeCount; i++)
        {
            if ((memory_requirements.memoryTypeBits & 1) == 1)
            {
                // Type is available, does it match user properties?
                if ((_device_memory_properties.memoryTypes[i].propertyFlags & property_bits) == property_bits)
                {
                    type_index_out = i;
                    return true;
                }
            }
            memory_requirements.memoryTypeBits >>= 1;
        }
        return false;
    }

    void VulkanPhysicalDevice::create_device(VulkanDeviceQueueType queue_type) noexcept
    {
        auto queue_typeid = static_cast<uint64_t>(queue_type);
        auto* it = core::pod::multi_hash::find_first(_device_factories, queue_typeid);
        if (it != nullptr)
        {
            auto& factory_data = it->value;

            if (queue_type == VulkanDeviceQueueType::GraphicsQueue)
            {
                VkDevice device_handle = factory_data.factory_function(_allocator, _physical_device_handle, factory_data.family_index);
                _graphics_device = core::memory::make_unique<VulkanDevice>(
                    _allocator,
                    _allocator,
                    queue_type,
                    factory_data.family_index,
                    device_handle,
                    factory_data.supports_present
                    );
            }
            else
            {
                IS_ASSERT(false, "Device creation for given queue type not supported!");
            }
        }
    }

    void VulkanPhysicalDevice::initialize() noexcept
    {
        enumerate_family_queues();
        enumerate_surface_capabilities();
        enumerate_surface_present_modes();
        enumerate_surface_formats();

        vkGetPhysicalDeviceProperties(_physical_device_handle, &_device_properties);
        vkGetPhysicalDeviceMemoryProperties(_physical_device_handle, &_device_memory_properties);

        create_device(VulkanDeviceQueueType::GraphicsQueue);
    }

    void VulkanPhysicalDevice::enumerate_family_queues() noexcept
    {
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(_physical_device_handle, &queue_family_count, nullptr);

        core::pod::Array<VkQueueFamilyProperties> queue_family_properties{ _allocator };
        core::pod::array::resize(queue_family_properties, queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(_physical_device_handle, &queue_family_count, &queue_family_properties[0]);

        // Going through all queue family properties.
        for (uint32_t index = 0; index < queue_family_count; ++index)
        {
            const auto& queue_family_property = queue_family_properties[index];

            // Check If family supports presenting
            VkBool32 supports_present = VK_FALSE;

            auto api_result = vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device_handle, index, _surface_handle, &supports_present);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query information if family {} (index) supports presenting!", index);

            // Check each familiy flags
            for (auto flag_bit : detail::queue_flags_array)
            {
                if (static_cast<VkQueueFlagBits>(queue_family_property.queueFlags & flag_bit) == flag_bit)
                {
                    if (auto* queue_factory = detail::factory_for_flag_bit(flag_bit); queue_factory != nullptr)
                    {
                        VulkanDeviceFactory factory{
                            .factory_function = queue_factory,
                            .family_properties = queue_family_property,
                            .family_index = VulkanQueueFamilyIndex{ index },
                            .supports_present = supports_present == VK_TRUE
                        };

                        auto queue_type = static_cast<uint64_t>(detail::queue_type_for_flag_bit(flag_bit));
                        core::pod::multi_hash::insert(_device_factories, queue_type, factory);
                    }
                }
            }
        }
    }

    void VulkanPhysicalDevice::enumerate_surface_capabilities() noexcept
    {
        auto api_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device_handle, _surface_handle, &_surface_capabilities);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get device surface capabilities!");
    }

    void VulkanPhysicalDevice::enumerate_surface_present_modes() noexcept
    {
        uint32_t present_mode_number;
        VkResult api_result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device_handle, _surface_handle, &present_mode_number, nullptr);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get number of device present modes!");

        core::pod::array::resize(_present_modes, present_mode_number);
        api_result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device_handle, _surface_handle, &present_mode_number, &_present_modes[0]);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query device present modes!");
    }

    void VulkanPhysicalDevice::enumerate_surface_formats() noexcept
    {
        uint32_t surface_format_number;
        VkResult api_result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device_handle, _surface_handle, &surface_format_number, nullptr);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get number of device surface formats!");

        core::pod::array::resize(_surface_formats, surface_format_number);
        api_result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device_handle, _surface_handle, &surface_format_number, &_surface_formats[0]);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query device surface formats!");
    }

    void VulkanPhysicalDevice::shutdown() noexcept
    {
        _graphics_device = nullptr;
    }

} // namespace render::vulkan
