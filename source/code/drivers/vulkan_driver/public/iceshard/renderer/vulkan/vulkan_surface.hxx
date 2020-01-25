#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanSurface : uintptr_t
    {
        Invalid = 0x0
    };

    [[nodiscard]]
    auto native_handle(VulkanSurface surface) noexcept -> VkSurfaceKHR;

    [[nodiscard]]
    auto create_surface(core::allocator& alloc, VkInstance vulkan_instance, VkExtent2D initial_extents) noexcept -> VulkanSurface;

    void destroy_surface(core::allocator& alloc, VulkanSurface surface) noexcept;

} // namespace iceshard::renderer::vulkan
