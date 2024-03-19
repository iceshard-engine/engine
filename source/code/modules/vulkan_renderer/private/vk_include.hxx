/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/windows.hxx>
#include <vulkan/vulkan.h>

constexpr nullptr_t vk_nullptr = VK_NULL_HANDLE;

// Instance Extension Functions
PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vk_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;

// Device Extension Functions
PFN_vkGetCalibratedTimestampsEXT vk_vkGetCalibratedTimestampsEXT = nullptr;

template<typename VkFnPtr, typename VkObj>
inline bool vk_get_proc_address(VkFnPtr& out_ptr, VkObj obj, char const* name) noexcept
{
    if constexpr(std::is_same_v<VkObj, VkDevice>)
    {
        out_ptr = reinterpret_cast<VkFnPtr>(vkGetDeviceProcAddr(obj, name));
    }
    else if constexpr (std::is_same_v<VkObj, VkInstance>)
    {
        out_ptr = reinterpret_cast<VkFnPtr>(vkGetInstanceProcAddr(obj, name));
    }
    else
    {
        out_ptr = nullptr;
    }
    return out_ptr != nullptr;
}
