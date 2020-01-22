#include "vulkan_surface.win32.hxx"

namespace iceshard::renderer::vulkan
{

    auto create_surface() noexcept -> VulkanSurface
    {
        return VulkanSurface::Invalid;
    }

    void destroy_surface(VulkanSurface) noexcept
    {
    }

} // namespace iceshard::renderer::vulkan
