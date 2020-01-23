#include <iceshard/renderer/vulkan/vulkan_viewport.hxx>
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanViewport::VulkanViewport(ViewportExtents) noexcept
        : _surface{ VulkanSurface::Invalid }
    {
    }

    VulkanViewport::~VulkanViewport() noexcept
    {
    }

    auto VulkanViewport::extents() const noexcept -> ViewportExtents
    {
        return { };
    }

    bool VulkanViewport::update_viewport(ViewportExtents) const noexcept
    {
        return false;
    }

} // namespace iceshard::renderer::vulkan
