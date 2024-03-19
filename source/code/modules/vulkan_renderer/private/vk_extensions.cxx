/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_extensions.hxx"
#include "vk_memory_allocator.hxx"

namespace ice::render::vk
{

    static constexpr ExtensionInfo Constant_SupportedExtensions[] = {
        { ExtensionTarget::InstanceLayer, Extension::VkDbg_ValidationLayer, 0, "VK_LAYER_KHRONOS_validation" },
        { ExtensionTarget::InstanceExtension, Extension::VkI_Surface, 0, VK_KHR_SURFACE_EXTENSION_NAME },
#if ISP_WINDOWS
        { ExtensionTarget::InstanceExtension, Extension::VkI_Win32Surface, 0, VK_KHR_WIN32_SURFACE_EXTENSION_NAME },
#elif ISP_ANDROID
        { ExtensionTarget::InstanceExtension, Extension::VkI_AndroidSurface, 0, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME },
#else
#error Unknown platform
#endif
        { ExtensionTarget::InstanceExtension, Extension::VkI_GetPhysicalDeviceProperties2, 0, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME },
        { ExtensionTarget::DeviceExtension, Extension::VkD_Swapchain, 0, VK_KHR_SWAPCHAIN_EXTENSION_NAME },
        { ExtensionTarget::DeviceExtension, Extension::VkD_DedicatedAllocation, 0, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME },
        { ExtensionTarget::DeviceExtension, Extension::VkD_MemoryBudget, 0, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME },
        { ExtensionTarget::DeviceExtension, Extension::VkD_GetMemoryRequirements2, 0, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME },
        { ExtensionTarget::DeviceExtension, Extension::VkD_CalibratedTimestamps, 0, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME },
        // Dependent extensions, only returns flags if the required extensions where found.
        { ExtensionTarget::VmaExtension, Extension::Vma_DedicatedAllocation, VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT, "" },
        { ExtensionTarget::VmaExtension, Extension::Vma_MemoryBudget, VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT, "" },
    };

    auto gather_instance_layers(ice::Array<ExtensionName>& out_names, ice::ucount& out_count) noexcept -> Extension
    {
        Extension result = Extension::None;

        // Get the layer count using a null pointer as the last parameter.
        uint32_t instance_layer_count = 0;
        vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);

        ice::Allocator& alloc = *out_names._allocator;
        // Enumerate layers with a valid pointer in the last parameter.
        VkLayerProperties* layer_props = alloc.allocate<VkLayerProperties>(instance_layer_count);
        VkResult vk_result = vkEnumerateInstanceLayerProperties(&instance_layer_count, layer_props);
        ICE_ASSERT_CORE(vk_result == VK_SUCCESS);

        out_count = 0;
        for (VkLayerProperties const& available : ice::Span{ layer_props, instance_layer_count })
        {
            for (ExtensionInfo const& supported : Constant_SupportedExtensions)
            {
                if (strcmp(available.layerName, supported.identifier) == 0)
                {
                    result |= supported.extension;
                    out_count += 1;
                    ice::array::push_back(out_names, supported.identifier);
                }
            }
        }

        alloc.deallocate(layer_props);
        return result;
    }

    auto gather_instance_extensions(ice::Array<ExtensionName>& out_names, ice::ucount& out_count) noexcept -> Extension
    {
        Extension result = Extension::None;

        // Get the layer count using a null pointer as the last parameter.
        uint32_t instance_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);

        ice::Allocator& alloc = *out_names._allocator;
        // Enumerate layers with a valid pointer in the last parameter.
        VkExtensionProperties* extension_props = alloc.allocate<VkExtensionProperties>(instance_extension_count);
        VkResult vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, extension_props);
        ICE_ASSERT_CORE(vk_result == VK_SUCCESS);

        out_count = 0;
        for (VkExtensionProperties const& available : ice::Span{ extension_props, instance_extension_count })
        {
            for (ExtensionInfo const& supported : Constant_SupportedExtensions)
            {
                if (strcmp(available.extensionName, supported.identifier) == 0)
                {
                    result |= supported.extension;
                    out_count += 1;
                    ice::array::push_back(out_names, supported.identifier);
                }
            }
        }

        alloc.deallocate(extension_props);
        return result;
    }

    auto gather_device_extensions(ice::Array<ExtensionName>& out_names, ice::ucount& out_count, VkPhysicalDevice physical_device) noexcept -> Extension
    {
        Extension result = Extension::None;

        // Get the layer count using a null pointer as the last parameter.
        uint32_t instance_extension_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &instance_extension_count, nullptr);

        ice::Allocator& alloc = *out_names._allocator;
        // Enumerate layers with a valid pointer in the last parameter.
        VkExtensionProperties* extension_props = alloc.allocate<VkExtensionProperties>(instance_extension_count);
        VkResult vk_result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &instance_extension_count, extension_props);
        ICE_ASSERT_CORE(vk_result == VK_SUCCESS);

        out_count = 0;
        for (VkExtensionProperties const& available : ice::Span{ extension_props, instance_extension_count })
        {
            for (ExtensionInfo const& supported : Constant_SupportedExtensions)
            {
                if (strcmp(available.extensionName, supported.identifier) == 0)
                {
                    result |= supported.extension;
                    out_count += 1;
                    ice::array::push_back(out_names, supported.identifier);
                }
            }
        }

        alloc.deallocate(extension_props);
        return result;
    }

    auto extensions_gather_names(ice::Array<ExtensionName>& out_names, ice::ucount& out_count, ExtensionTarget target) noexcept -> Extension
    {
        switch(target)
        {
        case ExtensionTarget::InstanceLayer:
            return gather_instance_layers(out_names, out_count);
        case ExtensionTarget::InstanceExtension:
            return gather_instance_extensions(out_names, out_count);
        default:
            return Extension::None;
        }
    }

    auto extensions_gather_names(ice::Array<ExtensionName>& out_names, ice::ucount &out_count, VkPhysicalDevice physical_device) noexcept -> Extension
    {
        return gather_device_extensions(out_names, out_count, physical_device);
    }

    auto extension_create_native_flags(Extension extensions, ExtensionTarget target) noexcept -> uint32_t
    {
        uint32_t result = 0;
        for (ExtensionInfo const& available : Constant_SupportedExtensions)
        {
            if (available.target == target && ice::has_all(extensions, available.extension))
            {
                result |= available.flags;
            }
        }
        return result;
    }

} // namespace ice::render::vk
