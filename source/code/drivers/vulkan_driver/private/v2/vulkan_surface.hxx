#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanSurface : uintptr_t
    {
        Invalid = 0x0
    };

    auto create_surface() noexcept -> VulkanSurface;

    void destroy_surface(VulkanSurface) noexcept;

} // namespace iceshard::renderer::vulkan
