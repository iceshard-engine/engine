/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    enum class Extension : ice::u32
    {
        None,

        // Core instance extensions
        VkI_Surface = 0x0000'0001,
        VkI_Win32Surface = 0x0000'0002, // The three surfaces are mutually exclusive so they may share the same value
        VkI_AndroidSurface = 0x0000'0002,
        VkI_WaylandSurface = 0x0000'0002,
        VkI_XLibSurface = 0x0000'0004,
        VkI_GetPhysicalDeviceProperties2 = 0x0000'0008,

        // Core device extensions
        VkD_Swapchain = 0x0000'0010,
        VkD_MemoryBudget = 0x0000'0020,
        VkD_DedicatedAllocation = 0x0000'0040,
        VkD_GetMemoryRequirements2 = 0x0000'0080,
        VkD_CalibratedTimestamps = 0x0000'0100,

        // Debug layers
        VkDbg_ValidationLayer = 0x8000'0000,

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
