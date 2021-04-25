#include "vk_driver.hxx"
#include "vk_render_surface.hxx"
#include "vk_device.hxx"
#include "vk_include.hxx"
#include "vk_utility.hxx"

#include <ice/pod/array.hxx>
#include <ice/assert.hxx>


namespace ice::render::vk
{

    namespace detail
    {

        auto to_string(VkPhysicalDeviceType type) noexcept -> ice::String;

    } // namespace detail

    VulkanRenderDriver::VulkanRenderDriver(
        ice::UniquePtr<VulkanAllocator> vk_alloc,
        VkInstance vk_instance
    ) noexcept
        : _allocator{ vk_alloc->backing_allocator() }
        , _vk_alloc{ ice::move(vk_alloc) }
        , _vk_instance{ vk_instance }
        , _vk_physical_device{ vk_nullptr }
        , _vk_queue_family_properties{ _allocator }
    {
        ice::pod::Array<VkPhysicalDevice> physical_devices{ _allocator };
        if (enumerate_objects(physical_devices, vkEnumeratePhysicalDevices, vk_instance))
        {
            VK_LOG(ice::LogSeverity::Warning, "Found more than one device! Picking first discrete GPU from list!\n");

            VkPhysicalDeviceProperties physical_device_properties;
            for (VkPhysicalDevice physical_device : physical_devices)
            {
                vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

                if (physical_device_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    _vk_physical_device = physical_device;
                    break;
                }
            }

            vkGetPhysicalDeviceMemoryProperties(
                _vk_physical_device,
                &_vk_physical_device_memory_properties
            );

            ICE_ASSERT(_vk_physical_device != vk_nullptr, "No discrete GPU device found!");

            VK_LOG(ice::LogSeverity::Info, "Physical device properties:");
            VK_LOG(ice::LogSeverity::Info, "- vendor.id: {}", physical_device_properties.vendorID);
            VK_LOG(ice::LogSeverity::Info, "- device.id: {}", physical_device_properties.deviceID);
            VK_LOG(ice::LogSeverity::Info, "- device.type: {}", detail::to_string(physical_device_properties.deviceType));
            VK_LOG(ice::LogSeverity::Info, "- device.name: {}", physical_device_properties.deviceName);
            VK_LOG(ice::LogSeverity::Info, "- version.driver: {}", physical_device_properties.driverVersion);
            VK_LOG(ice::LogSeverity::Info, "- version.api: {}", physical_device_properties.apiVersion);
        }

        if (enumerate_objects(_vk_queue_family_properties, vkGetPhysicalDeviceQueueFamilyProperties, _vk_physical_device))
        {
            VK_LOG(ice::LogSeverity::Debug, "Device has {} queue families", size(_vk_queue_family_properties));
        }
    }

    VulkanRenderDriver::~VulkanRenderDriver() noexcept
    {
        vkDestroyInstance(_vk_instance, _vk_alloc->vulkan_callbacks());
    }

    auto VulkanRenderDriver::render_api() const noexcept -> ice::render::RenderDriverAPI
    {
        return RenderDriverAPI::Vulkan;
    }

    auto VulkanRenderDriver::create_surface(
        ice::render::SurfaceInfo const& surface_info
    ) noexcept -> ice::render::RenderSurface*
    {
        ICE_ASSERT(
            surface_info.type == ice::render::SurfaceType::Win32_Window,
            "Unsupported surface type provided, accepting 'Win32_Window' surfaces only!"
        );

        VkWin32SurfaceCreateInfoKHR surface_create_info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        surface_create_info.hinstance = static_cast<HINSTANCE>(surface_info.win32.hinstance);
        surface_create_info.hwnd = static_cast<HWND>(surface_info.win32.hwn);

        VkSurfaceKHR vulkan_surface;
        auto api_result = vkCreateWin32SurfaceKHR(_vk_instance, &surface_create_info, nullptr, &vulkan_surface);
        ICE_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create Vulkan surface!");

        ice::i32 family_index = 0;
        for (VkQueueFamilyProperties const& queue_family_props : _vk_queue_family_properties)
        {
            VkBool32 supports_presenting;
            VkResult api_result = vkGetPhysicalDeviceSurfaceSupportKHR(
                _vk_physical_device,
                family_index,
                vulkan_surface,
                &supports_presenting
            );
            ICE_ASSERT(
                api_result == VkResult::VK_SUCCESS,
                "Couldn't query information if family {} (index) supports presenting!",
                family_index
            );

            if (supports_presenting == VK_TRUE)
            {
                if (_vk_presentation_queue_family_index == -1)
                {
                    _vk_presentation_queue_family_index = family_index;
                }
                else if ((queue_family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                    _vk_presentation_queue_family_index = family_index;
                }
            }

            family_index += 1;
        }

        return _vk_alloc->make<VulkanRenderSurface>(_vk_instance, vulkan_surface);
    }

    void VulkanRenderDriver::destroy_surface(
        ice::render::RenderSurface* surface
    ) noexcept
    {
        _vk_alloc->destroy(reinterpret_cast<VulkanRenderSurface*>(surface));
    }

    void VulkanRenderDriver::query_queue_infos(
        ice::pod::Array<ice::render::QueueFamilyInfo>& queue_info
    ) noexcept
    {
        ice::u32 queue_count = 0;
        for (VkQueueFamilyProperties const& queue_family_props : _vk_queue_family_properties)
        {
            queue_count += queue_family_props.queueCount;
        }

        ice::u32 queue_index = 0;
        ice::pod::array::reserve(queue_info, queue_count);
        for (VkQueueFamilyProperties const& queue_family_props : _vk_queue_family_properties)
        {
            QueueFlags flags = QueueFlags::Invalid;

            if (queue_family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                flags = flags | QueueFlags::Graphics;
            }
            if (queue_family_props.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                flags = flags | QueueFlags::Compute;
            }
            if (queue_family_props.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                flags = flags | QueueFlags::Transfer;
            }
            if (queue_index == _vk_presentation_queue_family_index)
            {
                flags = flags | QueueFlags::Present;
            }

            ice::pod::array::push_back(
                queue_info,
                QueueFamilyInfo{
                    .id = QueueID{ queue_index },
                    .flags = flags,
                    .count = queue_family_props.queueCount
                }
            );

            queue_index += 1;
        }
    }

    auto VulkanRenderDriver::create_device(
        ice::Span<ice::render::QueueInfo const> queue_info
    ) noexcept -> ice::render::RenderDevice*
    {
        static ice::f32 queue_priorities[] = {
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,
        };

        ice::pod::Array<VkDeviceQueueCreateInfo> queue_create_infos{ _allocator };
        ice::pod::array::reserve(queue_create_infos, 3);

        for (QueueInfo const& queue_info : queue_info)
        {
            ice::u32 const family_index = static_cast<ice::u32>(queue_info.id);

            VkDeviceQueueCreateInfo queue_create_info{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queue_create_info.queueFamilyIndex = family_index;
            queue_create_info.queueCount = queue_info.count;
            queue_create_info.pQueuePriorities = queue_priorities;

            ice::pod::array::push_back(queue_create_infos, queue_create_info);
        }

        char const* const extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkPhysicalDeviceFeatures available_device_features{ };
        vkGetPhysicalDeviceFeatures(_vk_physical_device, &available_device_features);

        VkPhysicalDeviceFeatures enabled_device_features{ };
        enabled_device_features.geometryShader = available_device_features.geometryShader;
        enabled_device_features.tessellationShader = available_device_features.tessellationShader;
        enabled_device_features.fillModeNonSolid = available_device_features.fillModeNonSolid;

        VkDeviceCreateInfo device_create_info{ .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_create_info.pEnabledFeatures = &enabled_device_features;
        device_create_info.enabledExtensionCount = ice::size(extension_names);
        device_create_info.ppEnabledExtensionNames = &extension_names[0];
        device_create_info.pQueueCreateInfos = ice::pod::array::begin(queue_create_infos);
        device_create_info.queueCreateInfoCount = ice::pod::array::size(queue_create_infos);

        VkDevice vk_device;
        VkResult result = vkCreateDevice(
            _vk_physical_device,
            &device_create_info,
            nullptr,
            &vk_device
        );

        ICE_ASSERT(
            result == VK_SUCCESS,
            "Couldn't create logical device"
        );

        return _allocator.make<VulkanRenderDevice>(
            _allocator,
            vk_device,
            _vk_physical_device,
            _vk_physical_device_memory_properties
        );
    }

    void VulkanRenderDriver::destroy_device(
        ice::render::RenderDevice* device
    ) noexcept
    {
        _allocator.destroy(static_cast<VulkanRenderDevice*>(device));
    }

    //bool VulkanRenderDriver::create_queues(
    //    ice::render::QueueCreateInfo const& queue_info
    //) noexcept
    //{
    //}

    //auto VulkanRenderDriver::get_queue(
    //    ice::render::QueueType type,
    //    ice::u32 queue_index
    //) noexcept -> ice::render::RenderQueue*
    //{
    //    if (type == QueueType::Present)
    //    {
    //        ICE_ASSERT(
    //            _vk_presentation_queue_family_index != -1,
    //            "Cannot create presentation queue without a render surface!"
    //        );
    //    }

    //}

    auto detail::to_string(VkPhysicalDeviceType type) noexcept -> ice::String
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

} // namespace ice::render::vk
