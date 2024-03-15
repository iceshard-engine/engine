/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    enum class Extension : ice::u16
    {
        None,

        // Core instance extensions
        VkI_Surface = 0x00'01,
        VkI_Win32Surface= 0x00'02,
        VkI_AndroidSurface= 0x00'04,
        VkI_GetPhysicalDeviceProperties2 = 0x00'08,

        // Core device extensions
        VkD_Swapchain = 0x00'10,
        VkD_MemoryBudget = 0x00'20,
        VkD_DedicatedAllocation = 0x00'40,
        VkD_GetMemoryRequirements2 = 0x00'80,

        // Debug layers
        VkDbg_ValidationLayer = 0x80'00,

        // VMA extensions
        Vma_MemoryBudget = VkI_GetPhysicalDeviceProperties2 | VkD_MemoryBudget,
        Vma_DedicatedAllocation = VkD_DedicatedAllocation | VkD_GetMemoryRequirements2,
    };

    enum class ExtensionTarget : ice::u8
    {
        InstanceLayer,
        InstanceExtension,
        DeviceExtension,
        VmaExtension
    };

    using ExtensionName = char const*;
    using ExtensionNativeFlags = uint32_t;

    struct ExtensionInfo
    {
        ExtensionTarget target;
        Extension extension;
        ExtensionNativeFlags flags;
        ExtensionName identifier;
    };

    auto extensions_gather_names(
        ice::Array<ExtensionName>& out_names,
        ice::ucount& out_count,
        ExtensionTarget target
    ) noexcept -> Extension;

    auto extensions_gather_names(
        ice::Array<ExtensionName>& out_names,
        ice::ucount& out_count,
        VkPhysicalDevice physical_device
    ) noexcept -> Extension;

    auto extension_create_native_flags(Extension extensions, ExtensionTarget target) noexcept -> uint32_t;

    inline constexpr auto operator==(Extension left, ExtensionInfo requirement) noexcept
    {
        return left == requirement.extension;
    }

} // namespace ice::render::vk
